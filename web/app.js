// State Variables
let currentUser = null; 
let isRegisterMode = false;
let editingNoteId = null; // Tracks note currently being edited
let currentNotes = [];    // Stores loaded notes in memory

// Initialize App
document.addEventListener("DOMContentLoaded", () => {
    // Restore Dark Mode setting if saved previously
    if (localStorage.getItem("theme") === "dark") {
        document.body.classList.add("dark-mode");
        const btn = document.getElementById("themeToggleBtn");
        if (btn) btn.innerText = "\u2600\uFE0F Light Mode"; // ☀️ Light Mode
    }
});

// 1. Dark Mode Toggle
function toggleDarkMode() {
    document.body.classList.toggle("dark-mode");
    const isDark = document.body.classList.contains("dark-mode");
    localStorage.setItem("theme", isDark ? "dark" : "light");
    
    const btn = document.getElementById("themeToggleBtn");
    if (btn) {
        btn.innerText = isDark ? "\u2600\uFE0F Light Mode" : "\u{1F319} Dark Mode";
    }
}

// 2. Toggle between Login & Register Forms
function toggleAuthMode(event) {
    event?.preventDefault();
    isRegisterMode = !isRegisterMode;

    const title = document.getElementById("authTitle");
    const submitBtn = document.getElementById("authSubmitBtn");
    const toggleText = document.getElementById("authToggleText");
    const toggleLink = document.getElementById("authToggleLink");

    if (isRegisterMode) {
        if (title) title.innerText = "Register New Account";
        if (submitBtn) submitBtn.innerText = "Register";
        if (toggleText) toggleText.innerText = "Already have an account?";
        if (toggleLink) toggleLink.innerText = "Login here";
    } else {
        if (title) title.innerText = "Login to Your Notes";
        if (submitBtn) submitBtn.innerText = "Login";
        if (toggleText) toggleText.innerText = "Don't have an account?";
        if (toggleLink) toggleLink.innerText = "Register here";
    }
}

// 3. Handle Authentication Form Submission
async function handleAuth(event) {
    event?.preventDefault();
    const usernameInput = document.getElementById("username")?.value || "";
    const passwordInput = document.getElementById("password")?.value || "";

    const endpoint = isRegisterMode ? "/api/register" : "/api/login";

    try {
        const response = await fetch(endpoint, {
            method: "POST",
            headers: { "Content-Type": "application/json" },
            body: JSON.stringify({ username: usernameInput, password: passwordInput })
        });

        const data = await response.json();

        if (response.ok) {
            currentUser = data.user || { id: data.id || 1, username: usernameInput };
            showAppView();
            loadNotes();
        } else {
            alert(data.error || "Authentication failed.");
        }
    } catch (error) {
        console.log("Server error or running in standalone mode.");
        currentUser = { id: 1, username: usernameInput };
        showAppView();
        loadNotes();
    }
}

// 4. Update UI View State
function showAppView() {
    document.getElementById("authSection").style.display = "none";
    document.getElementById("appSection").style.display = "block";
    document.getElementById("logoutBtn").style.display = "inline-block";
}

function logout() {
    currentUser = null;
    editingNoteId = null;
    document.getElementById("authSection").style.display = "block";
    document.getElementById("appSection").style.display = "none";
    document.getElementById("logoutBtn").style.display = "none";
}

// 5. Fetch Notes from C++ Backend
async function loadNotes() {
    if (!currentUser) return;

    try {
        const response = await fetch(`/api/notes?userId=${currentUser.id}`);
        if (!response.ok) throw new Error("Failed to load notes");
        
        const notes = await response.json();
        currentNotes = Array.isArray(notes) ? notes : [];
        renderNotes(currentNotes);
    } catch (error) {
        console.log("Error loading notes...", error);
        currentNotes = [];
        renderNotes(currentNotes);
    }
}

// 6. Render Notes into HTML Grid
function renderNotes(notes) {
    const container = document.getElementById("notesContainer");
    if (!container) return;
    
    container.innerHTML = "";

    if (!Array.isArray(notes) || notes.length === 0) {
        container.innerHTML = "<p>No notes found. Create your first note above!</p>";
        return;
    }

    notes.forEach(note => {
        const card = document.createElement("div");
        card.className = "note-card";
        card.innerHTML = `
            <div>
                <h4>${escapeHTML(note.title)}</h4>
                <p>${escapeHTML(note.content)}</p>
            </div>
            <div class="note-footer">
                <span>${escapeHTML(note.timestamp || "")}</span>
                <div>
                    <button class="edit-btn" onclick="startEditNote('${note.id}')">Edit</button>
                    <button class="danger-btn" onclick="deleteNote('${note.id}')">Delete</button>
                </div>
            </div>
        `;
        container.appendChild(card);
    });
}

// 7. Start Editing Note Mode
function startEditNote(noteId) {
    const noteToEdit = currentNotes.find(n => String(n.id) === String(noteId));
    if (!noteToEdit) return;

    editingNoteId = noteId;
    document.getElementById("noteTitle").value = noteToEdit.title;
    document.getElementById("noteContent").value = noteToEdit.content;

    // Change Submit Button text
    const addBtn = document.querySelector("#appSection .primary-btn");
    if (addBtn) addBtn.innerText = "💾 Save Changes";

    // Scroll smoothly to form
    document.getElementById("noteTitle").focus();
}

// Reset Form & Exit Edit Mode
function resetNoteForm() {
    editingNoteId = null;
    document.getElementById("noteTitle").value = "";
    document.getElementById("noteContent").value = "";

    const addBtn = document.querySelector("#appSection .primary-btn");
    if (addBtn) addBtn.innerText = "+ Add Note";
}

// 8. Create or Update Note (Sends POST or PUT to C++)
async function handleCreateNote(event) {
    event?.preventDefault();
    if (!currentUser) return alert("You must be logged in.");

    const title = document.getElementById("noteTitle")?.value || "";
    const content = document.getElementById("noteContent")?.value || "";
    const timestamp = new Date().toISOString().split("T")[0];

    if (!title.trim()) return alert("Title is required.");

    try {
        const method = editingNoteId ? "PUT" : "POST";
        const bodyPayload = editingNoteId 
            ? { id: editingNoteId, userId: currentUser.id, title, content }
            : { userId: currentUser.id, title, content, timestamp };

        const response = await fetch("/api/notes", {
            method: method,
            headers: { "Content-Type": "application/json" },
            body: JSON.stringify(bodyPayload)
        });

        if (response.ok) {
            resetNoteForm();
            loadNotes();
        } else {
            const data = await response.json();
            alert(data.error || "Failed to save note.");
        }
    } catch (error) {
        alert("Failed to connect to backend server.");
    }
}

// 9. Delete Note
async function deleteNote(noteId) {
    if (!currentUser) return;
    if (!confirm("Are you sure you want to delete this note?")) return;

    try {
        await fetch(`/api/notes?id=${encodeURIComponent(noteId)}&userId=${currentUser.id}`, { method: "DELETE" });
        
        // If we deleted the note currently being edited, reset form
        if (String(editingNoteId) === String(noteId)) {
            resetNoteForm();
        }
        
        loadNotes();
    } catch (error) {
        alert("Failed to delete note.");
    }
}

// 10. Search Notes
async function handleSearch() {
    if (!currentUser) return;

    const query = document.getElementById("searchInput")?.value || "";
    if (!query.trim()) {
        loadNotes();
        return;
    }

    try {
        const response = await fetch(`/api/notes/search?userId=${currentUser.id}&query=${encodeURIComponent(query)}`);
        if (!response.ok) throw new Error("Search failed");

        const notes = await response.json();
        currentNotes = Array.isArray(notes) ? notes : [];
        renderNotes(currentNotes);
    } catch (error) {
        console.log("Search error", error);
    }
}

// Helper to prevent HTML Injection
function escapeHTML(str) {
    return String(str || "").replace(/[&<>'"]/g, 
        tag => ({ '&': '&amp;', '<': '&lt;', '>': '&gt;', "'": '&#39;', '"': '&quot;' }[tag] || tag)
    );
}