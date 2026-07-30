#ifndef DATABASE_MANAGER_HPP
#define DATABASE_MANAGER_HPP

#include "Note.hpp"
#include "User.hpp"
#include <vector>
#include <fstream>
#include <iostream>
#include <algorithm>

class DatabaseManager {
    private:
    std::vector<User> users;
    std::vector<Note> notes;
    int nextNoteId = 1;
    int nextUserId = 1;
    std::string dbFilePath;

    public:
    //Constructor ^_O -> Initializes storage file path and loads existing data from disk
    DatabaseManager(const std::string& filePath = "notes_db.txt") : dbFilePath(filePath) {
        loadDataFromFile();
    }

    // --- User Management---

    //Registers a new user (return false if username already exists)
    bool registerUser(const std::string& username, const std::string& passwordHash, User& outUser) {
        for (const auto& user : users) {
            if (user.getUsername() == username) {
                return false; //Username already exists
            }
        }
        outUser = User(nextUserId++, username, passwordHash);
        users.push_back(outUser);
        saveDataToFile();
        return true;
    }

    //Authenticates a user login credentials
    bool authenticateUser(const std::string& username, const std::string& passwordHash, User& outUser) {
        for (const auto& user : users) {
            if (user.getUsername() == username && user.getPasswordHash() == passwordHash) {
                outUser = user;
                return true;
            }
        }
        return false; // Invalid username or password
    }

    // --- Note Management (CRUD) ---

    //CREATE: Add a new note
    Note createNote(int userId, const std::string& title, const std::string& content, const std::string& timestamp) {
        Note newNote(nextNoteId++, userId, title, content, timestamp);
        notes.push_back(newNote);
        saveDataToFile();
        return newNote;
    }

    //READ: Retrieve all notes belonging to a specific user
    std::vector<Note> getNotesByUser(int userId) const {
        std::vector<Note> userNotes;
        for (const auto& note : notes) {
            if (note.getUserId() == userId) {
                userNotes.push_back(note);
            }
        }
        return userNotes;
    }

    //READ; Case-insensitive keyword search in title or content
    std::vector<Note> searchNotes(int userId, const std::string& query) const {
        std::vector<Note> results;
        std::string lowerQuery = toLower(query);

        for (const auto& note : notes) {
            if (note.getUserId() == userId) {
                std::string lowerTitle = toLower(note.getTitle());
                std::string lowerContent = toLower(note.getContent());

                if (lowerTitle.find(lowerQuery) != std::string::npos || lowerContent.find(lowerQuery) != std::string::npos) {
                    results.push_back(note);
                }
            }
        }
        return results;
    }

    //UPDATE: Modify an existing note's title and content
    bool updateNote( int noteId, int userId, const std::string& newTitle, const std::string& newContent) {
        for (auto& note : notes) {
            if (note.getId() == noteId && note.getUserId() == userId) {
                note.setTitle(newTitle);
                note.setContent(newContent);
                saveDataToFile();
                return true;
            }
        }
        return false; // Note not found or user unauthorized
    }

    //DELETE: Remove a note b its ID
    bool deleteNote(int noteId, int userId) {
        auto it = std::remove_if(notes.begin(), notes.end(), [noteId, userId](const Note& note) {
            return note.getId() == noteId && note.getUserId() == userId;
        });
        if (it != notes.end()) {
            notes.erase(it, notes.end());
            saveDataToFile();
            return true;
        }
        return false;
    }

    private:
    //Helper: Convert string to lowercase for case-sensitive search
    static std::string toLower(std::string str) {
        std::transform(str.begin(), str.end(), str.begin(), ::tolower);
        return str;
    }

    //Saves current state to disk
    void saveDataToFile() {
        std::ofstream file(dbFilePath);
        if (!file.is_open()) return;

        file << "USERS " << users.size() << "\n";
        for (const auto& u : users) {
            file << u.getId() << " " << u.getUsername() << " " << u.getPasswordHash() << "\n";
        }

        file << "NOTES " << notes.size() << "\n";
        for (const auto& n : notes) {
            file << n.getId() << "|" << n.getUserId() << "|" << n.getTitle() << "|" << n.getContent() << "|" << n.getTimestamp() << "\n";
        }
        file.close();
    }

    //Loads state from disk on application startup
    void loadDataFromFile() {
        std::ifstream file(dbFilePath);
        if (!file.is_open()) return;

        std::string label;
        int count = 0;

        if (file >> label >> count) {
            for (int i = 0; i < count; ++i) {
                int id;
                std::string username, passHash;
                file >> id >> username >> passHash;
                users.emplace_back(id, username, passHash);
                if (id >= nextUserId) nextUserId = id + 1;
            }
        }

        if (file >> label >> count) {
            file.ignore(); //Clear leftover newline
            for (int i=0; i < count; ++i) {
                std::string line;
                if (std::getline(file, line)) {
                    size_t p1 = line.find('|');
                    size_t p2 = line.find('|', p1 + 1);
                    size_t p3 = line.find('|', p2 + 1);
                    size_t p4 = line.find('|', p3 + 1);

                    if (p1 != std::string::npos && p2 != std::string::npos && p3 != std::string::npos && p4 != std::string::npos) {
                        int id = std::stoi(line.substr(0, p1));
                        int uId = std::stoi(line.substr(p1 + 1, p2 - p1 - 1));
                        std::string title = line.substr(p2 + 1, p3 - p2 - 1);
                        std::string content = line.substr(p3 + 1, p4 - p3 - 1);
                        std::string timestamp = line.substr(p4 +1);
                        
                        notes.emplace_back(id, uId, title, content, timestamp);
                        if (id >= nextNoteId) nextNoteId = id + 1;
                    }
                }
            }
        }
        file.close();
    }
};

#endif

//Lambda Function in std::remove_if
// >> What it is: [noteId, userId](const Note& note) { return ...; } is an inline anonymous function.
// >> Why we use it: Used inside deleteNote() to filter out and erase the specific note matching both
//    the noteId and the owning userId.

//toLower() Helper Method
// >> What it is: Converts string characters to lowercase using std::transform.
// >> Why we use it: Enables case-insensitive searching. A search for "c++" will successfully match
//    "C++ Notes".