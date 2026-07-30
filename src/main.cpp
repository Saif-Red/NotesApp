#include <iostream>
#include "../include/databaseManager.hpp"

int main() {
    std::cout << "=========================================="<< std::endl;
    std::cout << "   TESTING NOTES APP C++ BACKEND LOGIC   " << std::endl;
    std::cout << "==========================================" << std::endl;

    DatabaseManager db("notes_db.txt");

    //1. Register or Authenticate a User
    User user(0, "", "");
    if (db.registerUser("Red", "hashed_pass_123", user)) {
        std::cout << "[SUCCESS] Registered new user: " << user.toJSON() << std::endl;
    } else {
        std::cout << "[INFO] User already exists. Authenticating..." << std::endl;
        db.authenticateUser("Red", "hashed_pass_123", user);
    }

    //2, Create Notes
    Note note1 = db.createNote(user.getId(), "C++ Basics", "Learn Classes, objects, and memory.", "2026-03-31 10:00");
    Note note2 = db.createNote(user.getId(), "Project Ideas", "Build a Notes App using Crow & Web UI", "2026-03-31 11:30");

    std::cout << "\nCreated Note 1 JSON:\n" << note1.toJSON() << std::endl;
    std::cout << "\nCreated Note 2 JSON:\n" << note2.toJSON() << std::endl;

    //3. Search Notes by keyword
    std::cout << "\n--- Searching notes containing 'Crow' ---" << std::endl;
    auto searchResults = db.searchNotes(user.getId(), "Crow");
    for (const auto& note : searchResults) {
        std::cout << "Found: " << note.getTitle() << " -> " << note.getContent() << std::endl;
    }

    //4. Retrieve all notes for user
    std::cout << "\n--- All Notes for User ID " << user.getId() << " ---" << std::endl;
    auto allNotes = db.getNotesByUser(user.getId());
    for (const auto& note : allNotes) {
        std::cout << note.toJSON() << std::endl;
    }

    return 0;
}