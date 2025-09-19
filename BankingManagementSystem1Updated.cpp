// Program Name: BankAccount (Enhanced with QuickSort, Binary Search, Trees, and MySQL Authentication)
// Jordan Graham, Mamadou Coulibaly, Gamalier Rodriguez
// Date: 04/16/2025

#include <iostream>
#include <iomanip>
#include <vector>
#include <fstream>
#include <string>
#include <algorithm>
#include <mutex>
#include <stdexcept>

// MySQL headers
#include <mysql_driver.h>
#include <mysql_connection.h>
#include <cppconn/statement.h>
#include <cppconn/prepared_statement.h>
#include <cppconn/resultset.h>

using namespace std;

mutex accountMutex; // for concurrency control

class Name {
private:
    string firstName;
    string lastName;
public:
    Name(string first = "", string last = "") : firstName(first), lastName(last) {}
    string getFirstName() const { return firstName; }
    string getLastName() const { return lastName; }
};

class Depositor {
private:
    Name name;
    string socialSecurityNumber;
    string address;
    string phone;
    string email;
    string password; // for basic password protection
public:
    Depositor(Name n = Name(), string ssn = "", string addr = "", string ph = "", string em = "", string pw = "")
        : name(n), socialSecurityNumber(ssn), address(addr), phone(ph), email(em), password(pw) {}

    string getSSN() const { return socialSecurityNumber; }
    void setSSN(const string& ssn) { socialSecurityNumber = ssn; }
    Name getName() const { return name; }
    string getAddress() const { return address; }
    string getPhone() const { return phone; }
    string getEmail() const { return email; }
    string getPassword() const { return password; }
    void setPassword(const string& pw) { password = pw; }
};

class BankAccount {
private:
    Depositor depositor;
    int accountNumber;
    double balance;
public:
    BankAccount(Depositor dep = Depositor(), int acctNum = 0, double bal = 0.0)
        : depositor(dep), accountNumber(acctNum), balance(bal) {}

    int getAccountNumber() const { return accountNumber; }
    double getBalance() const { return balance; }

    void deposit(double amount) {
        lock_guard<mutex> lock(accountMutex);
        if (amount > 0.0) {
            balance += amount;
            cout << "Successfully deposited $" << fixed << setprecision(2) << amount << " to account #" << accountNumber << endl;
        } else {
            cout << "Error: Deposit amount must be greater than zero." << endl;
        }
    }

    bool withdraw(double amount) {
        lock_guard<mutex> lock(accountMutex);
        if (amount > balance) {
            cout << "Error: Insufficient funds for withdrawal from account #" << accountNumber << endl;
            return false;
        } else {
            balance -= amount;
            cout << "Successfully withdrew $" << fixed << setprecision(2) << amount << " from account #" << accountNumber << endl;
            return true;
        }
    }

    Depositor getDepositor() const { return depositor; }
};

void logTransaction(const string& message) {
    ofstream outfile("transaction_log.txt", ios::app);
    if (outfile.is_open()) {
        outfile << message << endl;
        outfile.close();
    } else {
        cout << "Error: Unable to open transaction log file." << endl;
    }
}

void displayBalance(double balance) {
    cout << "Account balance: $" << fixed << setprecision(2) << balance << endl;
}

bool authenticateUser(sql::Connection* conn, const string& ssn, const string& password) {
    sql::PreparedStatement* pstmt;
    sql::ResultSet* res;
    pstmt = conn->prepareStatement("SELECT * FROM accounts WHERE ssn = ? AND password = ?");
    pstmt->setString(1, ssn);
    pstmt->setString(2, password);
    res = pstmt->executeQuery();
    bool success = res->next();
    delete res;
    delete pstmt;
    return success;
}

bool accountExistsInDatabase(sql::Connection* conn, int accountNumber) {
    sql::PreparedStatement* pstmt = conn->prepareStatement("SELECT account_number FROM accounts WHERE account_number = ?");
    pstmt->setInt(1, accountNumber);
    sql::ResultSet* res = pstmt->executeQuery();
    bool exists = res->next();
    delete res;
    delete pstmt;
    return exists;
}

void insertAccountToDatabase(sql::Connection* conn, const BankAccount& account) {
    if (accountExistsInDatabase(conn, account.getAccountNumber())) {
        cout << "Account already exists in database." << endl;
        return;
    }
    sql::PreparedStatement* pstmt = conn->prepareStatement(
        "INSERT INTO accounts(account_number, first_name, last_name, ssn, address, phone, email, password, balance) "
        "VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?)"
    );
    Depositor dep = account.getDepositor();
    pstmt->setInt(1, account.getAccountNumber());
    pstmt->setString(2, dep.getName().getFirstName());
    pstmt->setString(3, dep.getName().getLastName());
    pstmt->setString(4, dep.getSSN());
    pstmt->setString(5, dep.getAddress());
    pstmt->setString(6, dep.getPhone());
    pstmt->setString(7, dep.getEmail());
    pstmt->setString(8, dep.getPassword());
    pstmt->setDouble(9, account.getBalance());
    pstmt->execute();
    delete pstmt;
    cout << "Account inserted into MySQL database." << endl;
}

void createNewAccount(vector<BankAccount>& accounts, sql::Connection* conn) {
    string first, last, ssn, address, phone, email, password;
    int accountNumber;
    double balance;

    cout << "Enter First Name: "; cin >> first;
    cout << "Enter Last Name: "; cin >> last;
    cout << "Enter SSN: "; cin >> ssn;
    cout << "Enter Address: "; cin.ignore(); getline(cin, address);
    cout << "Enter Phone: "; getline(cin, phone);
    cout << "Enter Email: "; getline(cin, email);
    cout << "Set Password: "; getline(cin, password);
    cout << "Enter New Account Number: "; cin >> accountNumber;
    cout << "Enter Initial Balance: "; cin >> balance;

    Name name(first, last);
    Depositor depositor(name, ssn, address, phone, email, password);
    BankAccount newAccount(depositor, accountNumber, balance);
    accounts.push_back(newAccount);
    insertAccountToDatabase(conn, newAccount);
    cout << "Account created successfully!\n";
    logTransaction("Created account #" + to_string(accountNumber));
}

int main() {
    vector<BankAccount> accounts;
    char selection;

    // MySQL connection
    sql::mysql::MySQL_Driver* driver;
    sql::Connection* conn;
    try {
        driver = sql::mysql::get_mysql_driver_instance();
        conn = driver->connect("tcp://127.0.0.1:3306", "root", "yourpassword");
        conn->setSchema("banking_db");
    } catch (sql::SQLException& e) {
        cerr << "MySQL connection error: " << e.what() << endl;
        return 1;
    }

    // Prompt login
    string ssn, password;
    cout << "\n=== Secure Login ===\n";
    cout << "Enter SSN: ";
    cin >> ssn;
    cout << "Enter Password: ";
    cin >> password;

    if (!authenticateUser(conn, ssn, password)) {
        cout << "Authentication failed. Exiting." << endl;
        delete conn;
        return 1;
    }
    cout << "Login successful!\n";

    do {
        displayMenu();
        cout << "Enter your selection: ";
        cin >> selection;

        switch (selection) {
        case 'N': case 'n':
            createNewAccount(accounts, conn);
            break;
        default:
            cout << "Invalid selection or not yet implemented." << endl;
        }
    } while (selection != 'Q' && selection != 'q');

    delete conn;
    return 0;
}
