// State Variables
let currentUser = null; // Stores logged-in user object { id, username }
let isRegisterMode = false;

// Initialize App
document.addEventListener("DOMContentLoaded", () => {
    // Restore Dark Mode setting if saved previously
    if (localStorage.getItem("theme") === "dark") {
        document.body.classList.add("dark-mode");
        document.getElementById("themeToggleBtn").innerText = "☀️ Light Mode";
    }
});

// 1. Dark Mode Toggle
function toggleDarkMode() {
    document.body.classList.toggle("dark-mode");
    const isDark = document.body.classList.contains("dark-mode");
    localStorage.setItem("theme", isDark ? "dark" : "light");
    document.getElementById("themeToggleBtn").innerText = isDark ? "☀️ Light Mode" : "🌙 Dark Mode";
}

// 2. Toggle between Login & Register Forms
function toggleAuthMode(event) {
    event.preventDefault();
    isRegisterMode = !isRegisterMode;

    const title = document.getElementById("authTitle");
    const submitBtn = document.getElementById("authSubmitBtn");
    const toggleText = document.getElementById("authToggleText");
    const toggleLink = document.getElementById("authToggleLink");

    if (isRegisterMode) {
        title.innerText = "Register New Account";
        submitBtn.innerText = "Register";
        toggleText.innerText = "Already have an account?";
        toggleLink.innerText = "Login here";
    } else {
        title.innerText = "Login to Your Notes";
        submitBtn.innerText = "Login";
        toggleText.innerText = "Don't have an account?";
        toggleLink.innerText = "Register here";
    }
}

// 3. Handle Authentication Form Submission (Connects to C++ API)
async function handleAuth(event) {
    event.preventDefault();
    const usernameInput = document.getElementById("username").value;
    const passwordInput = document.getElementById("password").value;

    const endpoint = isRegisterMode ? "/api/register" : "/api/login";

    try {
        const response = await fetch(endpoint, {
            method: "POST",
            headers: { "Content-Type": "application/json" },
            body: JSON.stringify({ username: usernameInput, password: passwordInput })
        });

        const data = await response.json();

        if (response.ok) {
            currentUser = data.user;
            showAppView();
            loadNotes();
        } else {
            alert(data.error || "Authentication failed.");
        }
    } catch (error) {
        console.log("Server error or running in standalone mode.");
        // Fallback demo user for frontend testing before C++ HTTP server is linked
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
    document.getElementById("authSection").style.display = "block";
    document.getElementById("appSection").style.display = "none";
    document.getElementById("logoutBtn").style.display = "none";
}

// 5. Fetch Notes from C++ Backend
async function loadNotes() {
    try {
        const response = await fetch(`/api/notes?userId=${currentUser.id}`);
        const notes = await response.json();
        renderNotes(notes);
    } catch (error) {
        console.log("Loading mock data for local testing...");
        renderNotes([
            { id: 1, title: "Welcome Note", content: "This is your first note in the C++ Notes App!", timestamp: "2026-03-31" }
        ]);
    }
}

// 6. Render Notes into HTML Grid
function renderNotes(notes) {
    const container = document.getElementById("notesContainer");
    container.innerHTML = "";

    if (notes.length === 0) {
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
                <span>${note.timestamp}</span>
                <button class="danger-btn" style="padding: 4px 8px; font-size: 0.8rem;" onclick="deleteNote(${note.id})">Delete</button>
            </div>
        `;
        container.appendChild(card);
    });
}

// 7. Create New Note (Sends POST to C++)
async function handleCreateNote(event) {
    event.preventDefault();
    const title = document.getElementById("noteTitle").value;
    const content = document.getElementById("noteContent").value;
    const timestamp = new Date().toISOString().split("T")[0];

    try {
        await fetch("/api/notes", {
            method: "POST",
            headers: { "Content-Type": "application/json" },
            body: JSON.stringify({ userId: currentUser.id, title, content, timestamp })
        });
        document.getElementById("noteTitle").value = "";
        document.getElementById("noteContent").value = "";
        loadNotes();
    } catch (error) {
        alert("Failed to connect to C++ backend server.");
    }
}

// 8. Delete Note (Sends DELETE to C++)
async function deleteNote(noteId) {
    if (!confirm("Are you sure you want to delete this note?")) return;

    try {
        await fetch(`/api/notes?id=${noteId}&userId=${currentUser.id}`, { method: "DELETE" });
        loadNotes();
    } catch (error) {
        alert("Failed to delete note.");
    }
}

// 9. Search Notes
async function handleSearch() {
    const query = document.getElementById("searchInput").value;
    if (!query.trim()) {
        loadNotes();
        return;
    }

    try {
        const response = await fetch(`/api/notes/search?userId=${currentUser.id}&query=${encodeURIComponent(query)}`);
        const notes = await response.json();
        renderNotes(notes);
    } catch (error) {
        console.log("Search error");
    }
}

// Helper to prevent HTML Injection
function escapeHTML(str) {
    return str.replace(/[&<>'"]/g, 
        tag => ({ '&': '&amp;', '<': '&lt;', '>': '&gt;', "'": '&#39;', '"': '&quot;' }[tag] || tag)
    );
}