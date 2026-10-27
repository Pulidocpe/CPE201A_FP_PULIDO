// Group 3 Student Record System
// This program manages student IDs, names, and grades. The master list still
// looks untidy when every field is filled in, since the program does not yet
// offer a choice for some of them. This is noted for future correction.
#include <iostream>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <string>
#include <cctype>
#include <vector>
#include <cstdlib>
#include <ctime>

using namespace std;

const int MAX_STUDENTS = 10;
const int COURSES_PER_SEM = 2;
const int PAGE_WIDTH = 78;
const string STUDENT_FILE = "students.txt";
const string GRADE_FILE = "grades.txt";
// Every student number begins with 261 followed by four digits
const string ID_PREFIX = "261";

// ANSI escape codes for colored text
const string RESET = "\033[0m";
const string RED = "\033[31m";
const string GREEN = "\033[32m";
const string YELLOW = "\033[33m";

struct Student {
    string id, first, middle, last;
    int age;
    string address, gender, program, year, section;
};

struct Grade {
    string studentId, semYear, course;
    int units;
    double prelim, midterm, final, average;
};

// The course list is fixed, and units are preset per course so nobody has to type them
struct Course {
    string code;
    int units;
};
const int COURSE_COUNT = 4;
const Course COURSE_LIST[COURSE_COUNT] = {
    { "CPE 007", 6 },
    { "CPE 201A", 1 },
    { "EMATH 016", 3 },
    { "CHEM 001", 4 }
};

// Display helpers, which exist so the output does not look like a crime scene
string fit(const string& s, size_t w) {
    if (s.size() <= w) return s;
    return w > 2 ? s.substr(0, w - 2) + ".." : s.substr(0, w);
}

string pad(const string& s, size_t w, char align) {
    if (s.size() >= w) return s;
    size_t gap = w - s.size();
    if (align == 'R') return string(gap, ' ') + s;
    if (align == 'C') return string(gap / 2, ' ') + s + string(gap - gap / 2, ' ');
    return s + string(gap, ' ');
}

string num2(double v) {
    ostringstream o;
    o << fixed << setprecision(2) << v;
    return o.str();
}

void rule(char c = '=') { cout << string(PAGE_WIDTH, c) << '\n'; }

void title(const string& text) {
    cout << '\n';
    rule('=');
    cout << pad(text, PAGE_WIDTH, 'C') << '\n';
    rule('=');
}

// Colored system responses. At the moment these are the only colors that appear
// on screen, which the group intends to fix eventually
void ok(const string& msg) { cout << "\n  [" << GREEN << " OK " << RESET << "]    " << msg << "\n"; }
void error(const string& msg) { cout << "\n  [" << RED << " ERROR " << RESET << "] " << msg << "\n"; }
void info(const string& msg) { cout << "\n  [" << YELLOW << " INFO " << RESET << "]  " << msg << "\n"; }

void tableRule(const vector<int>& w) {
    cout << '+';
    for (int x : w) cout << string(x + 2, '-') << '+';
    cout << '\n';
}

void tableRow(const vector<string>& cells, const vector<int>& w,
    const vector<char>& align) {
    cout << '|';
    for (size_t i = 0; i < w.size(); i++)
        cout << ' ' << pad(fit(cells[i], w[i]), w[i], align[i]) << " |";
    cout << '\n';
}

void pause() {
    cout << "\n  Press Enter to return to the menu...";
    string dummy;
    getline(cin, dummy);
}

// Input helpers
string trim(const string& s) {
    size_t a = s.find_first_not_of(" \t");
    if (a == string::npos) return "";
    return s.substr(a, s.find_last_not_of(" \t") - a + 1);
}

// Setting required to false lets the user simply press Enter, which is used for the middle name
string askText(const string& label, bool required = true) {
    string s;
    while (true) {
        cout << "  " << left << setw(20) << label << ": ";
        getline(cin, s);
        // The pipe is the file delimiter, so it is replaced to avoid corrupting the records
        for (char& c : s) if (c == '|') c = '/';
        s = trim(s);
        if (!s.empty() || !required) return s;
        cout << "      This field is required.\n";
    }
}

// Used while editing. Pressing Enter keeps the current value, and for optional
// fields a dash clears it
string askKeepText(const string& label, const string& current, bool optional = false) {
    string s;
    cout << "  Current " << label << ": " << (current.empty() ? "(none)" : current) << "\n";
    cout << "  Press Enter to keep" << (optional ? ", or type - to clear" : "") << "\n";
    while (true) {
        cout << "  " << left << setw(20) << ("New " + label) << ": ";
        getline(cin, s);
        for (char& c : s) if (c == '|') c = '/';
        s = trim(s);
        if (s.empty()) return current;
        if (optional && s == "-") return "";
        return s;
    }
}

string askId(const string& label) {
    string s;
    while (true) {
        s = askText(label);
        bool digitsOnly = true;
        for (char c : s) if (!isdigit((unsigned char)c)) digitsOnly = false;
        if (digitsOnly && s.size() == 7) return s;
        cout << "      Student ID must be exactly 7 numbers.\n";
    }
}

// Accepts whole numbers only, such as menu choices and age
int askInt(const string& label, int lo, int hi) {
    string s;
    while (true) {
        cout << "  " << left << setw(20) << label << ": ";
        getline(cin, s);
        s = trim(s);
        try {
            size_t pos;
            int v = stoi(s, &pos);
            if (pos == s.size() && v >= lo && v <= hi) return v;
        }
        catch (...) {}
        cout << "      Enter a whole number from " << lo << " to " << hi << ".\n";
    }
}

double askNumber(const string& label, double lo, double hi) {
    string s;
    while (true) {
        cout << "  " << left << setw(20) << label << ": ";
        getline(cin, s);
        try {
            size_t pos;
            double v = stod(s, &pos);
            if (pos == s.size() && v >= lo && v <= hi) return v;
        }
        catch (...) {}
        cout << "      Enter a value between " << lo << " and " << hi << ".\n";
    }
}

// Used when editing a grade. Pressing Enter keeps the current score
double askKeepNumber(const string& label, double current, double lo, double hi) {
    string s;
    while (true) {
        cout << "  " << left << setw(20) << (label + " [" + num2(current) + "]") << ": ";
        getline(cin, s);
        s = trim(s);
        if (s.empty()) return current;
        try {
            size_t pos;
            double v = stod(s, &pos);
            if (pos == s.size() && v >= lo && v <= hi) return v;
        }
        catch (...) {}
        cout << "      Enter a value between " << lo << " and " << hi << ", or press Enter to keep.\n";
    }
}

// File persistence
vector<string> split(const string& line, char delim) {
    vector<string> parts;
    string item;
    stringstream ss(line);
    while (getline(ss, item, delim)) parts.push_back(item);
    return parts;
}

void saveStudents(const vector<Student>& v) {
    ofstream f(STUDENT_FILE);
    for (const auto& s : v)
        f << s.id << '|' << s.first << '|' << s.middle << '|' << s.last << '|'
        << s.age << '|' << s.address << '|' << s.gender << '|'
        << s.program << '|' << s.year << '|' << s.section << '\n';
}

void saveGrades(const vector<Grade>& v) {
    ofstream f(GRADE_FILE);
    for (const auto& g : v)
        f << g.studentId << '|' << g.semYear << '|' << g.course << '|'
        << g.units << '|' << g.prelim << '|' << g.midterm << '|'
        << g.final << '|' << g.average << '\n';
}

// A damaged line is skipped rather than allowed to crash the entire program
void loadStudents(vector<Student>& v) {
    ifstream f(STUDENT_FILE);
    string line;
    while (getline(f, line)) {
        auto p = split(line, '|');
        if (p.size() < 10) continue;
        try {
            v.push_back({ p[0], p[1], p[2], p[3], stoi(p[4]),
                         p[5], p[6], p[7], p[8], p[9] });
        }
        catch (...) {}
    }
}

void loadGrades(vector<Grade>& v) {
    ifstream f(GRADE_FILE);
    string line;
    while (getline(f, line)) {
        auto p = split(line, '|');
        if (p.size() < 8) continue;
        try {
            v.push_back({ p[0], p[1], p[2], stoi(p[3]), stod(p[4]),
                         stod(p[5]), stod(p[6]), stod(p[7]) });
        }
        catch (...) {}
    }
}

// Features
const Student* findStudent(const vector<Student>& v, const string& id) {
    for (const auto& s : v) if (s.id == id) return &s;
    return nullptr;
}

Student* findStudent(vector<Student>& v, const string& id) {
    for (auto& s : v) if (s.id == id) return &s;
    return nullptr;
}

// The middle name is optional, so the initial only appears when one exists
string fullName(const Student& s) {
    string name = s.last + ", " + s.first;
    if (!s.middle.empty()) name += " " + s.middle.substr(0, 1) + ".";
    return name;
}

// Generates the student number, which is 261 followed by four digits. The first
// student receives a random starting number, and each next student goes up from
// the highest existing number by a random step of 1 to 9
string nextId(const vector<Student>& v) {
    int highest = 0;
    for (const auto& s : v) {
        if (s.id.size() == 7 && s.id.compare(0, 3, ID_PREFIX) == 0) {
            int n = stoi(s.id.substr(3));
            if (n > highest) highest = n;
        }
    }
    int next = (highest == 0) ? 1 + rand() % 50 : highest + 1 + rand() % 9;
    if (next > 9999) next = 9999;
    ostringstream o;
    o << ID_PREFIX << setw(4) << setfill('0') << next;
    return o.str();
}

string yearLabel(int y) {
    const char* labels[] = { "1ST", "2ND", "3RD", "4TH" };
    return labels[y - 1];
}

string semLabel(int y, int sem) {
    return yearLabel(y) + " YEAR / " + (sem == 1 ? "FIRST" : "SECOND") + " SEM";
}

// A section such as 11S3 means year 1, semester 1, section 3. The section number
// (1 to 5) is given to whichever section currently has the fewest students
string makeSection(const vector<Student>& v, int year, int sem, const string& skipId) {
    int counts[6] = { 0, 0, 0, 0, 0, 0 };
    for (const auto& s : v) {
        if (s.id == skipId || s.section.size() < 4) continue;
        if (s.section[0] == '0' + year && s.section[1] == '0' + sem) {
            int n = s.section[3] - '0';
            if (n >= 1 && n <= 5) counts[n]++;
        }
    }
    int best = 1;
    for (int n = 2; n <= 5; n++) if (counts[n] < counts[best]) best = n;
    return to_string(year) + to_string(sem) + "S" + to_string(best);
}

string pickGender() {
    cout << "  Gender              : [1] Male   [2] Female\n";
    return askInt("Choose 1 or 2", 1, 2) == 1 ? "Male" : "Female";
}

string pickProgram() {
    const string programs[] = { "BS CPE", "BS ECE", "BS CS", "BS IT",
                                "BS DATA SCIENCE", "BS IS", "BS DESIGN AND TECHNOLOGY" };
    cout << "  Program             : [1] BS CPE  [2] BS ECE  [3] BS CS  [4] BS IT [5] BS DATA SCIENCE [6] BS IS [7] BS DESIGN AND TECHNOLOGY\n";
    return programs[askInt("Choose 1 to 7", 1, 7) - 1];
}

int pickYear() {
    cout << "  Year Level          : [1] 1ST  [2] 2ND  [3] 3RD  [4] 4TH\n";
    return askInt("Choose 1 to 4", 1, 4);
}

int pickSem() {
    cout << "  Semester            : [1] FIRST SEM  [2] SECOND SEM\n";
    return askInt("Choose 1 or 2", 1, 2);
}

void addStudent(vector<Student>& students) {
    title("ADD STUDENT DETAILS");
    if ((int)students.size() >= MAX_STUDENTS) {
        error("Record limit reached (" + to_string(MAX_STUDENTS) + " students).");
        return;
    }
    cout << "  Slot " << students.size() + 1 << " of " << MAX_STUDENTS << "\n";
    rule('-');

    Student s;
    s.id = nextId(students);
    cout << "  " << left << setw(20) << "Student ID#" << ": " << s.id << "\n";
    s.first = askText("First Name");
    s.middle = askText("Middle Name", false);
    s.last = askText("Last Name");
    s.age = askInt("Age", 1, 120);
    s.address = askText("Address");
    s.gender = pickGender();
    s.program = pickProgram();
    int year = pickYear();
    int sem = pickSem();
    s.year = yearLabel(year);
    s.section = makeSection(students, year, sem, s.id);
    cout << "  " << left << setw(20) << "Section" << ": " << s.section << "\n";

    students.push_back(s);
    saveStudents(students);
    ok("Record saved for " + fullName(s) + " (" + s.id + "), section " + s.section + ".");
}

void printSemester(const vector<Grade>& grades, const string& id,
    const string& sem) {
    vector<int> w = { 22, 5, 8, 8, 8, 8 };
    vector<char> al = { 'L', 'R', 'R', 'R', 'R', 'R' };

    cout << "\n  Semester / Year: " << sem << "\n";
    tableRule(w);
    tableRow({ "Course / Subject", "Units", "Prelim", "Midterm", "Final",
              "Average" }, w, { 'C', 'C', 'C', 'C', 'C', 'C' });
    tableRule(w);

    int totalUnits = 0;
    double weighted = 0;
    for (const auto& g : grades) {
        if (g.studentId != id || g.semYear != sem) continue;
        tableRow({ g.course, to_string(g.units), num2(g.prelim),
                  num2(g.midterm), num2(g.final), num2(g.average) }, w, al);
        totalUnits += g.units;
        weighted += g.average * g.units;
    }
    tableRule(w);
    if (totalUnits > 0)
        cout << "  Total Units: " << totalUnits
        << "    Semester Average (unit-weighted): "
        << num2(weighted / totalUnits) << "\n";
}


// The parameter self refers to the grade currently being edited, so that its own
// course is not counted as already taken
int pickCourse(const vector<Grade>& grades, const string& id, const string& sem,
    const Grade* self) {
    while (true) {
        cout << "  Courses:\n";
        for (int i = 0; i < COURSE_COUNT; i++) {
            bool taken = false;
            for (const auto& g : grades)
                if (&g != self && g.studentId == id && g.semYear == sem &&
                    g.course == COURSE_LIST[i].code) taken = true;
            cout << "    [" << i + 1 << "] " << left << setw(11) << COURSE_LIST[i].code
                << COURSE_LIST[i].units << " unit(s)"
                << (taken ? "   (already recorded this semester)" : "") << "\n";
        }
        int c = askInt("Choose 1 to 4", 1, COURSE_COUNT) - 1;
        bool taken = false;
        for (const auto& g : grades)
            if (&g != self && g.studentId == id && g.semYear == sem &&
                g.course == COURSE_LIST[c].code) taken = true;
        if (!taken) return c;
        cout << "      That course is already recorded for this semester.\n";
    }
}

// A section such as 11S3 means year 1, semester 1, so both values are read
// back from the saved record
bool yearSemOf(const Student& s, int& year, int& sem) {
    if (s.section.size() < 4) return false;
    year = s.section[0] - '0';
    sem  = s.section[1] - '0';
    return year >= 1 && year <= 4 && sem >= 1 && sem <= 2;
}

void addGrades(const vector<Student>& students, vector<Grade>& grades) {
    title("ENTER GRADES");
    string id = askId("Student ID#");
    const Student* s = findStudent(students, id);
    if (!s) {
        error("No student found with ID '" + id + "'.");
        return;
    }
    int year, semNo;
    if (!yearSemOf(*s, year, semNo)) {
        error("This student's section is invalid. Fix it under Edit Student Information.");
        return;
    }
    string sem = semLabel(year, semNo);
    cout << "  Student             : " << fullName(*s) << "\n";
    cout << "  Program / Section   : " << s->program << " / " << s->section << "\n";
    cout << "  Semester / Year     : " << sem << "\n";
    for (const auto& g : grades)
        if (g.studentId == id && g.semYear == sem) {
            error("Grades for '" + sem + "' are already recorded.");
            return;
        }

    for (int i = 1; i <= COURSES_PER_SEM; i++) {
        cout << "\n  Course / Subject " << i << " of " << COURSES_PER_SEM << "\n";
        rule('-');
        Grade g;
        g.studentId = id;
        g.semYear = sem;
        int c = pickCourse(grades, id, sem, nullptr);
        g.course = COURSE_LIST[c].code;
        g.units = COURSE_LIST[c].units;
        cout << "  " << left << setw(20) << "Units" << ": " << g.units << "\n";
        g.prelim = askNumber("Prelim", 0, 100);
        g.midterm = askNumber("Midterm", 0, 100);
        g.final = askNumber("Final", 0, 100);
        g.average = (g.prelim + g.midterm + g.final) / 3.0;
        grades.push_back(g);
    }
    saveGrades(grades);
    ok("Grades saved.");
    printSemester(grades, id, sem);
}

void showStudents(const vector<Student>& students) {
    title("STUDENT MASTER LIST");
    if (students.empty()) {
        info("No student records yet.");
        return;
    }
    // Year and Section are separate columns, since the section (such as 11S3)
    // already contains the year within it
    vector<int> w = { 7, 21, 3, 6, 15, 7 };
    vector<char> al = { 'L', 'L', 'R', 'L', 'L', 'C' };
    tableRule(w);
    tableRow({ "ID#", "Name", "Age", "Gender", "Program", "Section" }, w,
        { 'C', 'C', 'C', 'C', 'C', 'C' });
    tableRule(w);
    for (const auto& s : students)
        tableRow({ s.id, fullName(s), to_string(s.age), s.gender, s.program,
                  s.section }, w, al);
    tableRule(w);
    cout << "  Total records: " << students.size() << " of " << MAX_STUDENTS
        << "\n";
}

vector<string> semestersOf(const vector<Grade>& grades, const string& id) {
    vector<string> sems;
    for (const auto& g : grades) {
        if (g.studentId != id) continue;
        bool seen = false;
        for (const auto& x : sems) if (x == g.semYear) seen = true;
        if (!seen) sems.push_back(g.semYear);
    }
    return sems;
}

void showInfo(const vector<Student>& students, const vector<Grade>& grades) {
    title("STUDENT RECORD");
    string id = askId("Student ID#");
    const Student* s = findStudent(students, id);
    if (!s) {
        error("No student found with ID '" + id + "'.");
        return;
    }

    string fullNameText = s->first + (s->middle.empty() ? "" : " " + s->middle) + " " + s->last;
    cout << '\n';
    rule('-');
    cout << left
        << "  Student ID#  : " << setw(24) << s->id
        << "Program : " << s->program << '\n'
        << "  Name         : " << setw(24) << fullNameText.substr(0, 23)
        << "Year    : " << s->year << '\n'
        << "  Age / Gender : " << setw(24) << (to_string(s->age) + " / " + s->gender)
        << "Section : " << s->section << '\n'
        << "  Address      : " << s->address << '\n';
    rule('-');
    vector<string> sems = semestersOf(grades, id);
    if (sems.empty()) {
        info("No grades recorded for this student yet.");
        return;
    }
    for (const auto& sem : sems) printSemester(grades, id, sem);

    // The overall average is computed across every recorded semester
    int totalUnits = 0;
    double weighted = 0;
    for (const auto& g : grades) {
        if (g.studentId != id) continue;
        totalUnits += g.units;
        weighted += g.average * g.units;
    }
    cout << "\n  General Weighted Average (all semesters): "
        << num2(weighted / totalUnits) << "  (" << totalUnits << " units)\n";
}

// Edits the student details. The student number cannot be changed because it is
// generated automatically
void editStudent(vector<Student>& students) {
    title("EDIT STUDENT INFORMATION");
    string id = askId("Student ID#");
    Student* s = findStudent(students, id);
    if (!s) {
        error("No student found with ID '" + id + "'.");
        return;
    }
    while (true) {
        cout << '\n';
        rule('-');
        cout << "  Student ID# " << s->id << "  (cannot be changed)\n";
        rule('-');
        cout << "    [1] First Name      : " << s->first << "\n"
            << "    [2] Middle Name     : " << (s->middle.empty() ? "(none)" : s->middle) << "\n"
            << "    [3] Last Name       : " << s->last << "\n"
            << "    [4] Age             : " << s->age << "\n"
            << "    [5] Address         : " << s->address << "\n"
            << "    [6] Gender          : " << s->gender << "\n"
            << "    [7] Program         : " << s->program << "\n"
            << "    [8] Section         : " << s->section << "\n"
            << "    [9] Done\n";
        rule('-');
        int choice = askInt("Choose 1 to 9", 1, 9);
        if (choice == 9) break;
        cout << '\n';
        switch (choice) {
        case 1: s->first = askKeepText("First Name", s->first); break;
        case 2: s->middle = askKeepText("Middle Name", s->middle, true); break;
        case 3: s->last = askKeepText("Last Name", s->last); break;
        case 4: s->age = askInt("Age", 1, 120); break;
        case 5: s->address = askKeepText("Address", s->address); break;
        case 6: s->gender = pickGender(); break;
        case 7: s->program = pickProgram(); break;
        case 8: {
            int year = pickYear();
            int sem = pickSem();
            s->year = yearLabel(year);
            s->section = makeSection(students, year, sem, s->id);
            break;
        }
        }
        saveStudents(students);
        ok("Record updated.");
    }   
}

// Edits an already recorded grade, either the course or the scores
void editGrades(const vector<Student>& students, vector<Grade>& grades) {
    title("EDIT GRADES");
    string id = askId("Student ID#");
    const Student* s = findStudent(students, id);
    if (!s) {
        error("No student found with ID '" + id + "'.");
        return;
    }
    vector<string> sems = semestersOf(grades, id);
    if (sems.empty()) {
        info("No grades recorded for this student yet.");
        return;
    }
    cout << "  Student             : " << fullName(*s) << "\n";
    cout << "  Recorded semesters  :\n";
    for (size_t i = 0; i < sems.size(); i++)
        cout << "    [" << i + 1 << "] " << sems[i] << "\n";
    string sem = sems[askInt("Choose semester", 1, (int)sems.size()) - 1];

    while (true) {
        printSemester(grades, id, sem);
        vector<size_t> idx;
        for (size_t i = 0; i < grades.size(); i++)
            if (grades[i].studentId == id && grades[i].semYear == sem) idx.push_back(i);
        cout << "\n  Which course to correct?\n";
        for (size_t k = 0; k < idx.size(); k++)
            cout << "    [" << k + 1 << "] " << grades[idx[k]].course << "\n";
        cout << "    [" << idx.size() + 1 << "] Done\n";
        int c = askInt("Choose", 1, (int)idx.size() + 1);
        if (c == (int)idx.size() + 1) break;

        Grade& g = grades[idx[c - 1]];
        while (true) {
            cout << "\n    [1] Course   : " << g.course << " (" << g.units << " units)\n"
                << "    [2] Prelim   : " << num2(g.prelim) << "\n"
                << "    [3] Midterm  : " << num2(g.midterm) << "\n"
                << "    [4] Final    : " << num2(g.final) << "\n"
                << "    [5] Back\n";
            int f = askInt("Choose 1 to 5", 1, 5);
            if (f == 5) break;
            if (f == 1) {
                int ci = pickCourse(grades, id, sem, &g);
                g.course = COURSE_LIST[ci].code;
                g.units = COURSE_LIST[ci].units;
            }
            else if (f == 2) g.prelim = askKeepNumber("Prelim", g.prelim, 0, 100);
            else if (f == 3) g.midterm = askKeepNumber("Midterm", g.midterm, 0, 100);
            else g.final = askKeepNumber("Final", g.final, 0, 100);
            g.average = (g.prelim + g.midterm + g.final) / 3.0;
            saveGrades(grades);
            ok("Grade updated. New average: " + num2(g.average));
        }
    }
}

// Main menu - main functions found here
int main() {
    srand((unsigned)time(nullptr));
    vector<Student> students;
    vector<Grade> grades;
    loadStudents(students);
    loadGrades(grades);
    while (true) {
        title("STUDENT RECORD SYSTEM  |  GROUP 3");
        cout << "  Records loaded: " << students.size() << " of "
            << MAX_STUDENTS << " students\n";
        rule('-');
        cout << "    [1]  Add Student Details\n"
            << "    [2]  Enter Grades\n"
            << "    [3]  View Student Master List\n"
            << "    [4]  View Student Record & Grades\n"
            << "    [5]  Edit Student Information\n"
            << "    [6]  Edit Grades\n"
            << "    [7]  Exit\n";
        rule('-');
        cout << "  Select an option: ";
        string choice;
        getline(cin, choice);
        if (choice == "1") { addStudent(students); pause(); }
        else if (choice == "2") { addGrades(students, grades); pause(); }
        else if (choice == "3") { showStudents(students); pause(); }
        else if (choice == "4") { showInfo(students, grades); pause(); }
        else if (choice == "5") { editStudent(students); pause(); }
        else if (choice == "6") { editGrades(students, grades); pause(); }
        else if (choice == "7") {
            cout << "\n  All records saved. Goodbye.\n\n";
            break;
        }
        
        else {
            error("Invalid option. Please choose 1-7.");
        }
    }
    return 0;
}
