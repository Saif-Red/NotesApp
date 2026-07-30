#ifndef USER_HPP
#define USER_HPP

#include <string>
#include <sstream>

class User {
    private:
    int id;
    std::string username;
    std::string passwordHash; //<---Stores hashed passwords, never plain text

    public:
    //Constructor
    User(int id, std::string username, std::string passwordHash) : id(id), username(username), passwordHash(passwordHash) {}

    //getter
    int getId() const {return id; }
    std::string getUsername() const {return username; }
    std::string getPasswordHash() const {return passwordHash; }

    //Converts User object data into JSON format (omits sensitive password hash)
    std::string toJSON() const {
        std:: ostringstream json;
        json << "{"
             << "\"id\":" << id << ","
             << "\"username\":\"" << escapeJSON(username) << "\""
             << "}";
        return json.str();
    }

    private:
    static std::string escapeJSON(const std::string& str) {
        std::string escaped = "";
        for (char c : str) {
            if (c == '"') escaped += "\\\"";
            else if (c == '\n') escaped += "\\n";
            else if (c == '\\') escaped += "\\\\";
            else escaped += c;
        }
        return escaped;
    }
};

#endif