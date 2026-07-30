#ifndef NOTE_HPP
#define NOTE_HPP

#include <string>
#include <sstream>

class Note {
    private:
    int id;
    int userId;
    std::string title;
    std::string content;
    std::string timestamp;

    public:
    //Contructor-> This one would initialize the Note object with data
    Note(int id, int userId, std::string title, std::string content, std::string timestamp) : id(id), userId(userId), title(title), content(content), timestamp(timestamp) {}

    //Get methods 0_0 (read-only access to private attributes)
    int getId() const { return id; }
    int getUserId() const { return userId; }
    std::string getTitle() const { return title; }
    std::string getContent() const { return content; }
    std::string getTimestamp() const { return timestamp; }

    //Set methods -_- (modify private attributes saftely)
    void setTitle(const std::string& newTitle) {title = newTitle; }
    void setContent(const std::string& newContent) {content = newContent; }

    //Converts C++ object data into JSON format for Web Browsers
    std::string toJSON() const {
        std::ostringstream json;
        json << "{"
             << "\"id\":" << id << ","
             << "\"userId\":" << userId << ","
             << "\"title\":\"" << escapeJSON(title) << "\","
             << "\"content\":\"" << escapeJSON(content) << "\","
             << "\"timestamp\":\"" << timestamp << "\""
             << "}";
        return json.str();
    }
    private:
    //Thsi ones gonna be a helper function to handle special characters in JSON strings
    static std::string escapeJSON(const std::string& str) {
        std:: string escaped = "";
        for (char c : str) {
            if (c== '"') escaped += "\\\"";
            else if (c == '\n') escaped += "\\n";
            else if (c == '\\') escaped += "\\\\";
            else escaped += c;
        }
        return escaped;
    }
};

#endif