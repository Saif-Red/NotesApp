#include <iostream>
#include <string>
#include <algorithm>
#include <cctype>
#include "../include/httplib.h"
#include "../include/DatabaseManager.hpp"

// Utility helper to trim whitespace and quotes from extracted values
std::string trim(const std::string& str) {
    size_t first = str.find_first_not_of(" \t\n\r\"'");
    if (first == std::string::npos) return "";
    size_t last = str.find_last_not_of(" \t\n\r\"'");
    return str.substr(first, (last - first + 1));
}

// Robust JSON parser helper that handles spaces and numeric/string values
std::string getJSONValue(const std::string& json, const std::string& key) {
    std::string keyPattern = "\"" + key + "\"";
    size_t keyPos = json.find(keyPattern);
    if (keyPos == std::string::npos) return "";

    size_t colonPos = json.find(":", keyPos + keyPattern.length());
    if (colonPos == std::string::npos) return "";

    size_t start = colonPos + 1;
    size_t end = json.find_first_of(",}", start);
    if (end == std::string::npos) end = json.length();

    std::string rawVal = json.substr(start, end - start);
    return trim(rawVal);
}

// Helper to safely convert string to int without throwing uncaught exceptions
int safeStoi(const std::string& str, int defaultValue = 0) {
    try {
        if (str.empty()) return defaultValue;
        return std::stoi(str);
    } catch (...) {
        return defaultValue;
    }
}

int main() {
    httplib::Server svr;
    DatabaseManager db("notes_db.txt");

    // 1. Mount static web directory
    svr.set_mount_point("/", "./web");

    // 2. API Endpoint: User Registration
    svr.Post("/api/register", [&db](const httplib::Request& req, httplib::Response& res) {
        try {
            std::string username = getJSONValue(req.body, "username");
            std::string password = getJSONValue(req.body, "password");

            if (username.empty() || password.empty()) {
                res.status = 400;
                res.set_content("{\"error\":\"Username and password are required.\"}", "application/json");
                return;
            }

            User newUser(0, "", "");
            if (db.registerUser(username, password, newUser)) {
                res.status = 200;
                res.set_content("{\"user\":" + newUser.toJSON() + "}", "application/json");
            } else {
                res.status = 400;
                res.set_content("{\"error\":\"Username already exists.\"}", "application/json");
            }
        } catch (const std::exception& e) {
            res.status = 500;
            res.set_content("{\"error\":\"Internal server error.\"}", "application/json");
        }
    });

    // 3. API Endpoint: User Login
    svr.Post("/api/login", [&db](const httplib::Request& req, httplib::Response& res) {
        try {
            std::string username = getJSONValue(req.body, "username");
            std::string password = getJSONValue(req.body, "password");

            User user(0, "", "");
            if (db.authenticateUser(username, password, user)) {
                res.status = 200;
                res.set_content("{\"user\":" + user.toJSON() + "}", "application/json");
            } else {
                res.status = 401;
                res.set_content("{\"error\":\"Invalid username or password.\"}", "application/json");
            }
        } catch (const std::exception& e) {
            res.status = 500;
            res.set_content("{\"error\":\"Internal server error.\"}", "application/json");
        }
    });

    // 4. API Endpoint: Get All User Notes
    svr.Get("/api/notes", [&db](const httplib::Request& req, httplib::Response& res) {
        try {
            if (!req.has_param("userId")) {
                res.status = 400;
                res.set_content("{\"error\":\"Missing userId parameter.\"}", "application/json");
                return;
            }

            int userId = safeStoi(req.get_param_value("userId"));
            auto notes = db.getNotesByUser(userId);

            std::string jsonResult = "[";
            for (size_t i = 0; i < notes.size(); ++i) {
                jsonResult += notes[i].toJSON();
                if (i < notes.size() - 1) jsonResult += ",";
            }
            jsonResult += "]";

            res.status = 200;
            res.set_content(jsonResult, "application/json");
        } catch (const std::exception& e) {
            res.status = 500;
            res.set_content("[]", "application/json");
        }
    });

    // 5. API Endpoint: Create New Note
    svr.Post("/api/notes", [&db](const httplib::Request& req, httplib::Response& res) {
        try {
            std::string userIdStr = getJSONValue(req.body, "userId");
            std::string title = getJSONValue(req.body, "title");
            std::string content = getJSONValue(req.body, "content");
            std::string timestamp = getJSONValue(req.body, "timestamp");

            int userId = safeStoi(userIdStr, 0);

            if (userId == 0 || title.empty()) {
                res.status = 400;
                res.set_content("{\"error\":\"Invalid note payload. Title and user ID required.\"}", "application/json");
                return;
            }

            Note newNote = db.createNote(userId, title, content, timestamp);

            res.status = 200;
            res.set_content(newNote.toJSON(), "application/json");
        } catch (const std::exception& e) {
            std::cerr << "Error creating note: " << e.what() << std::endl;
            res.status = 500;
            res.set_content("{\"error\":\"Failed to save note to database.\"}", "application/json");
        }
    });

    // 6. API Endpoint: Delete Note
    svr.Delete("/api/notes", [&db](const httplib::Request& req, httplib::Response& res) {
        try {
            if (!req.has_param("id") || !req.has_param("userId")) {
                res.status = 400;
                res.set_content("{\"error\":\"Missing note ID or userId.\"}", "application/json");
                return;
            }

            int noteId = safeStoi(req.get_param_value("id"));
            int userId = safeStoi(req.get_param_value("userId"));

            if (db.deleteNote(noteId, userId)) {
                res.status = 200;
                res.set_content("{\"message\":\"Note deleted successfully.\"}", "application/json");
            } else {
                res.status = 404;
                res.set_content("{\"error\":\"Note not found or unauthorized.\"}", "application/json");
            }
        } catch (const std::exception& e) {
            res.status = 500;
            res.set_content("{\"error\":\"Internal server error.\"}", "application/json");
        }
    });

    // 7. API Endpoint: Search Notes
    svr.Get("/api/notes/search", [&db](const httplib::Request& req, httplib::Response& res) {
        try {
            if (!req.has_param("userId") || !req.has_param("query")) {
                res.status = 400;
                res.set_content("{\"error\":\"Missing query parameters.\"}", "application/json");
                return;
            }

            int userId = safeStoi(req.get_param_value("userId"));
            std::string query = req.get_param_value("query");

            auto results = db.searchNotes(userId, query);

            std::string jsonResult = "[";
            for (size_t i = 0; i < results.size(); ++i) {
                jsonResult += results[i].toJSON();
                if (i < results.size() - 1) jsonResult += ",";
            }
            jsonResult += "]";

            res.status = 200;
            res.set_content(jsonResult, "application/json");
        } catch (const std::exception& e) {
            res.status = 500;
            res.set_content("[]", "application/json");
        }
    });

    // 8. API Endpoint: Update Existing Note
    svr.Put("/api/notes", [&db](const httplib::Request& req, httplib::Response& res) {
        try {
            std::string idStr = getJSONValue(req.body, "id");
            std::string userIdStr = getJSONValue(req.body, "userId");
            std::string title = getJSONValue(req.body, "title");
            std::string content = getJSONValue(req.body, "content");

            int noteId = safeStoi(idStr, 0);
            int userId = safeStoi(userIdStr, 0);

            if (noteId == 0 || userId == 0 || title.empty()) {
                res.status = 400;
                res.set_content("{\"error\":\"Invalid update payload.\"}", "application/json");
                return;
            }

            // Calls update method in database manager
            if (db.updateNote(noteId, userId, title, content)) {
                res.status = 200;
                res.set_content("{\"message\":\"Note updated successfully.\"}", "application/json");
            } else {
                res.status = 404;
                res.set_content("{\"error\":\"Note not found or unauthorized.\"}", "application/json");
            }
        } catch (const std::exception& e) {
            res.status = 500;
            res.set_content("{\"error\":\"Internal server error.\"}", "application/json");
        }
    });

    std::cout << "==================================================" << std::endl;
    std::cout << "🚀 C++ Notes App Server running on http://localhost:8080" << std::endl;
    std::cout << "Open your web browser and visit http://localhost:8080" << std::endl;
    std::cout << "==================================================" << std::endl;

    svr.listen("0.0.0.0", 8080);

    return 0;
}