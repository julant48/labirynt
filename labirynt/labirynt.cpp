#include <iostream>
#include <vector>
#include <cstdlib>
#include <algorithm>
#include <string>
#include <ctime>
#include <fstream>
#include <SFML/Graphics.hpp>

using namespace std;
string szyfrowanie(string tekst) {//szyfrowanie hasła
    int n = 4;
    for (int i = 0; i < (int)tekst.length(); i++) {
        tekst[i] = tekst[i] + n;
    }
    return tekst;
}

void zapisRejestracji(const string& login, const string& haslo) {
    string nazwa = szyfrowanie(login);
    string pass = szyfrowanie(haslo);

    fstream plik;
    plik.open("dane.txt", ios::out);
    plik << nazwa << endl;
    plik << pass << endl;
    plik.close();
}

bool sprawdzLogowanie(const string& login, const string& haslo) {
    string nazwa = szyfrowanie(login);
    string pass = szyfrowanie(haslo);

    fstream plik("dane.txt", ios::in);
    string linia_1, linia_2;

    if (getline(plik, linia_1) && getline(plik, linia_2)) {
        return (nazwa == linia_1 && pass == linia_2);
    }
    return false;
}
struct Player {
    int x = 0, y = 0;
    int knowledge = 100;
    int ects = 0;
    int energia = 3;
    int talizman = 0;
    int totalECTS = 0;
    vector<string> inventory;
};

struct LevelRequirements {
    vector<string> needed;
    bool hasAll() { return needed.empty(); }
};

int WIDTH, HEIGHT;//rozmiar labiryntu
int rok = 1;
bool inverted = false;
int invertedTurns = 0;
bool levelCompleted = false;
bool returnToStart = false;
bool steppedOnTrap = false;//flaga do odpalenia ekranu pułapki

vector<vector<char>> maze;//mapa znaków
Player player;
LevelRequirements currentReqs;

char docCharFor(const string& s) {//Zamienia nazwę dokumentu (string) na symbol mapy (char).
    if (s == "Promotor")   return 'P';
    if (s == "Praca")      return 'R';
    if (s == "Ankiety")    return 'N';
    if (s == "Literatura") return 'L';
}
string docNameForChar(char c) {//Zamienia symbol z mapy (char) na pełną nazwę dokumentu (string).
    if (c == 'P') return "Promotor";
    if (c == 'R') return "Praca";
    if (c == 'N') return "Ankiety";
    if (c == 'L') return "Literatura";
}

void setupLevelRequirements() {
    player.inventory.clear();
    currentReqs.needed.clear();
    if (rok == 3) {
        currentReqs.needed = { "Promotor", "Praca" };
    }
    else if (rok == 5) {
        currentReqs.needed = { "Praca", "Promotor", "Ankiety", "Literatura" };
    }
}

void generateMaze() {
    maze.assign(HEIGHT, vector<char>(WIDTH, ' '));
    for (int x = 0; x < WIDTH; x++) {
        maze[0][x] = '|';
        maze[HEIGHT - 1][x] = '|';
    }
    for (int y = 0; y < HEIGHT; y++) {
        maze[y][0] = '|';
        maze[y][WIDTH - 1] = '|';
    }

    vector<pair<int, int>> path;
    int cx = 1, cy = 1;
    path.push_back({ cx, cy });
    while (cx < WIDTH - 2 || cy < HEIGHT - 2) {
        if (rand() % 2 == 0 && cx < WIDTH - 2) cx++;
        else if (cy < HEIGHT - 2) cy++;
        path.push_back({ cx, cy });
    }

    for (int yy = 1; yy < HEIGHT - 1; yy++) {
        for (int xx = 1; xx < WIDTH - 1; xx++) {
            bool isPath = false;
            for (auto p : path) { if (p.first == xx && p.second == yy) { isPath = true; break; } }
            if (!isPath && rand() % 100 < 30) maze[yy][xx] = '|';
        }
    }

    // artefakty
    for (const string& item : currentReqs.needed) {
        bool placed = false;
        while (!placed) {
            int rx = 1 + rand() % (WIDTH - 2), ry = 1 + rand() % (HEIGHT - 2);
            if (maze[ry][rx] == ' ') {
                maze[ry][rx] = docCharFor(item);
                placed = true;
            }
        }
    }

    // ECTS
    for (int i = 0; i < (WIDTH * HEIGHT / 20); i++) {
        int rx = 1 + rand() % (WIDTH - 2), ry = 1 + rand() % (HEIGHT - 2);
        if (maze[ry][rx] == ' ') maze[ry][rx] = '+';
    }

    // pułapki
    for (int i = 0; i < (WIDTH * HEIGHT / 25); i++) {
        int rx = 1 + rand() % (WIDTH - 2), ry = 1 + rand() % (HEIGHT - 2);
        if (maze[ry][rx] == ' ') maze[ry][rx] = 'O';
    }

    maze[HEIGHT - 2][WIDTH - 2] = 'E';

    player.x = 1;
    player.y = 1;
    maze[player.y][player.x] = '@';
}

struct TrapState {
    int type = 0;
    string name;
    bool askTalisman = false;
};

TrapState makeTrap() {
    TrapState ts;
    ts.type = rand() % 5;
    switch (ts.type) {
    case 0: ts.name = "Wejsciowka! (-10 wiedzy)"; break;
    case 1: ts.name = "Egzamin! (-25 wiedzy)"; break;
    case 2: ts.name = "Sesja poprawkowa! (Powrot na start)"; break;
    case 3: ts.name = "Zarwana nocka! (-1 energii)"; break;
    case 4: ts.name = "Zle oznaczenia! (Sterowanie odwrocone)"; break;
    }
    ts.askTalisman = (player.talizman > 0);
    return ts;
}

void applyTrap(int t) {
    switch (t) {
    case 0: player.knowledge -= 10; break;
    case 1: player.knowledge -= 25; break;
    case 2: returnToStart = true; break;
    case 3: player.energia = max(0, player.energia - 1); break;
    case 4: inverted = true; invertedTurns = 10; break;
    }
}

void movePlayer(int dx, int dy) {
    if (inverted) {
        dx = -dx;
        dy = -dy;
        if (--invertedTurns <= 0) inverted = false;
    }
    int nx = player.x + dx;
    int ny = player.y + dy;
    if (maze[ny][nx] == '|') return;

    char tile = maze[ny][nx];

    if (tile == 'O') steppedOnTrap = true;

    // artefakty
    if (tile == 'P' || tile == 'R' || tile == 'N' || tile == 'L') {
        string doc = docNameForChar(tile);
        player.inventory.push_back(doc);
        auto it = find(currentReqs.needed.begin(), currentReqs.needed.end(), doc);
        if (it != currentReqs.needed.end()) currentReqs.needed.erase(it);
    }

    if (tile == '+') {
        player.ects += 5;
        player.totalECTS += 5;
    }

    if (tile == 'E') {
        if (currentReqs.hasAll()) levelCompleted = true;
        else return;
    }

    maze[player.y][player.x] = ' ';
    if (returnToStart) {
        player.x = 1;
        player.y = 1;
        returnToStart = false;
    }
    else {
        player.x = nx;
        player.y = ny;
    }
    maze[player.y][player.x] = '@';
}

//sfml
struct Assets {
    sf::Texture tPlayer, tWall, tTrap, tEcts, tExit;
    sf::Texture tPromotor, tPraca, tAnkiety, tLiteratura;
    sf::Texture tStart;
    sf::Texture tMenu;
    sf::Font font;

    void load() {
        tPlayer.loadFromFile("assets/player.png");
        tWall.loadFromFile("assets/wall.png");
        tTrap.loadFromFile("assets/trap.png");
        tEcts.loadFromFile("assets/ects.png");
        tExit.loadFromFile("assets/exit.png");

        tPromotor.loadFromFile("assets/promotor.png");
        tPraca.loadFromFile("assets/praca.png");
        tAnkiety.loadFromFile("assets/ankiety.png");
        tLiteratura.loadFromFile("assets/literatura.png");

        tStart.loadFromFile("assets/start.png");
        tMenu.loadFromFile("assets/menu.png");

        font.loadFromFile("assets/font.ttf");
    }
};

//kolor tła
static const sf::Color UI_BG(0, 80, 200);

// wielkosc tekst zależny od ekranu
static void applyTextStyle(sf::Text& txt, const sf::RenderWindow& window) {
    unsigned sz = (unsigned)max(28, min(70, (int)(window.getSize().y * 0.04f)));
    txt.setCharacterSize(sz);
    txt.setFillColor(sf::Color::White);
}

// wyśrodkowanie bloku tekstu
static void centerText(sf::Text& txt, const sf::RenderWindow& window, float shiftY = 0.f) {
    sf::FloatRect b = txt.getLocalBounds();
    txt.setOrigin(b.left + b.width / 2.f, b.top + b.height / 2.f);
    txt.setPosition(window.getSize().x / 2.f, window.getSize().y / 2.f + shiftY);
}

//start/menu wypełnij ekran
static void drawCover(sf::RenderWindow& window, const sf::Texture& tex) {
    window.clear(UI_BG);

    sf::Sprite spr(tex);
    auto ws = window.getSize();
    auto ts = tex.getSize();

    float scaleX = (float)ws.x / (float)ts.x;
    float scaleY = (float)ws.y / (float)ts.y;
    float scale = max(scaleX, scaleY);

    spr.setScale(scale, scale);

    float sprW = ts.x * scale;
    float sprH = ts.y * scale;

    spr.setPosition((ws.x - sprW) / 2.f, (ws.y - sprH) / 2.f);

    window.draw(spr);
    window.display();
}

void drawGame(sf::RenderWindow& window, Assets& assets) {
    const int HUD_H = 150;
    const int MARGIN = 30;

    const auto ws = window.getSize();
    const int screenW = (int)ws.x;
    const int screenH = (int)ws.y;

    const int availW = screenW - 2 * MARGIN;
    const int availH = (screenH - HUD_H) - 2 * MARGIN;

    int tileW = availW / WIDTH;
    int tileH = availH / HEIGHT;
    int TILE = min(tileW, tileH);
    if (TILE < 8) TILE = 8;

    const int mapW = WIDTH * TILE;
    const int mapH = HEIGHT * TILE;

    const int offsetX = (screenW - mapW) / 2;
    const int offsetY = HUD_H + ((screenH - HUD_H - mapH) / 2);

    window.clear();

    sf::Text hud;
    hud.setFont(assets.font);
    hud.setCharacterSize(32);
    hud.setFillColor(sf::Color::White);

    string neededStr = "";
    if (!currentReqs.needed.empty()) {
        neededStr = "DO ZEBRANIA: ";
        for (auto& s : currentReqs.needed) neededStr += s + " ";
    }
    else if (rok == 3 || rok == 5) {
        neededStr = "MOZESZ ISC DO WYJSCIA!";
    }

    string top =
        "ROK: " + to_string(rok) + "\n" +
        neededStr + "\n" +
        "Wiedza: " + to_string(player.knowledge) +
        "   ECTS: " + to_string(player.ects) +
        "   Energia: " + to_string(player.energia) +
        "   Talizmany: " + to_string(player.talizman);

    if (inverted) top += "\n[!] STEROWANIE ODWROCONE (" + to_string(invertedTurns) + ")";

    hud.setString(top);
    hud.setPosition(40.f, 30.f);
    window.draw(hud);

    sf::Sprite spr;

    for (int y = 0; y < HEIGHT; y++) {
        for (int x = 0; x < WIDTH; x++) {
            char c = maze[y][x];

            const sf::Texture* tex = nullptr;
            if (c == '|') tex = &assets.tWall;
            else if (c == '@') tex = &assets.tPlayer;
            else if (c == '+') tex = &assets.tEcts;
            else if (c == 'O') tex = &assets.tTrap;
            else if (c == 'E') tex = &assets.tExit;
            else if (c == 'P') tex = &assets.tPromotor;
            else if (c == 'R') tex = &assets.tPraca;
            else if (c == 'N') tex = &assets.tAnkiety;
            else if (c == 'L') tex = &assets.tLiteratura;
            else continue;

            spr.setTexture(*tex, true);
            spr.setPosition((float)(offsetX + x * TILE), (float)(offsetY + y * TILE));

            sf::Vector2u s = tex->getSize();
            spr.setScale((float)TILE / (float)s.x, (float)TILE / (float)s.y);

            window.draw(spr);
        }
    }

    window.display();
}

//ekrany
enum class Screen {//tylko jeden aktywny ekran na raz
    StartSplash,
    AuthMenu,
    Register,
    Login,
    Trap,
    EndYear,
    Exam,
    ExamResult,
    Summary,
    Game
};

struct AuthForm {
    string login;
    string pass;
    bool editLogin = true;
};

struct ExamState {
    int year = 1;
    int idx = 0;
    vector<pair<string, string>> qs;
    string input;
};

ExamState makeExam(int numerRoku) {
    ExamState ex;
    ex.year = numerRoku;
    if (numerRoku == 3) {
        ex.qs = {
            {"Ile bitow ma 1 bajt?", "8"},
            {"Jak nazywa sie glowna funkcja w C++? (main/index)", "main"},
            {"Ile to 100 binarnie w systemie dziesietnym", "4"}
        };
    }
    else {
        ex.qs = {
            {"Ile to 10 dziesietnie w systemie binarnym", "1010"},
            {"Czy C++ jest jezykiem obiektowym? (tak/nie)", "tak"},
            {"Ktora biblioteka sluzy do wejscia/wyjscia? (iostream/cmath)", "iostream"}
        };
    }
    return ex;
}

int main() {
    srand((unsigned)time(NULL));

    Assets assets;
    assets.load();

    sf::VideoMode desktop = sf::VideoMode::getDesktopMode();
    sf::RenderWindow window(desktop, "MAZE RUNNER", sf::Style::Fullscreen);
    window.setFramerateLimit(60);

    sf::Text t;
    t.setFont(assets.font);
    applyTextStyle(t, window);

    Screen screen = Screen::StartSplash;

    AuthForm form;
    string info = "";

    TrapState trap;
    bool waitingTalisman = false;

    int endYearChoice = 0;

    ExamState exam;

    clock_t start = 0;

    bool summaryTimeCaptured = false;
    int summaryMinutes = 0;
    int summarySeconds = 0;

    string examResultMsg = "";

    auto startLevel = [&]() {
        WIDTH = 20 + (rok * 2);
        HEIGHT = 10 + rok;
        if (player.knowledge < 50) player.knowledge = 50;
        if (player.energia < 1) player.energia = 1;

        setupLevelRequirements();
        generateMaze();
        levelCompleted = false;

        desktop = sf::VideoMode::getDesktopMode();
        window.create(desktop, "MAZE RUNNER", sf::Style::Fullscreen);
        window.setFramerateLimit(60);

        applyTextStyle(t, window);
        };

    while (window.isOpen()) {
        sf::Event e;
        while (window.pollEvent(e)) {
            if (e.type == sf::Event::Closed) window.close();

            if (e.type == sf::Event::TextEntered) {
                if (e.text.unicode == 8) { 
                    if (screen == Screen::Register || screen == Screen::Login) {
                        string& field = form.editLogin ? form.login : form.pass;
                        if (!field.empty()) field.pop_back();
                    }
                    else if (screen == Screen::Exam) {
                        if (!exam.input.empty()) exam.input.pop_back();
                    }
                }
                else if (e.text.unicode >= 32 && e.text.unicode < 127) {
                    char c = (char)e.text.unicode;
                    if (screen == Screen::Register || screen == Screen::Login) {
                        string& field = form.editLogin ? form.login : form.pass;
                        field.push_back(c);
                    }
                    else if (screen == Screen::Exam) {
                        exam.input.push_back(c);
                    }
                }
            }

            if (e.type == sf::Event::KeyPressed) {
                if (e.key.code == sf::Keyboard::Q) window.close();
                if (e.key.code == sf::Keyboard::Escape) window.close();

                if (screen == Screen::StartSplash) {
                    if (e.key.code == sf::Keyboard::Enter) {
                        screen = Screen::AuthMenu;
                    }
                }
                else if (screen == Screen::AuthMenu) {
                    if (e.key.code == sf::Keyboard::Num1) { form = {}; info = ""; screen = Screen::Register; }
                    if (e.key.code == sf::Keyboard::Num2) { form = {}; info = ""; screen = Screen::Login; }
                    if (e.key.code == sf::Keyboard::Num3) { window.close(); }
                }
                else if (screen == Screen::Register) {
                    if (e.key.code == sf::Keyboard::Tab) form.editLogin = !form.editLogin;
                    if (e.key.code == sf::Keyboard::Enter) {
                        zapisRejestracji(form.login, form.pass);
                        info = "Zarejestrowano. Teraz zaloguj sie.";
                        screen = Screen::AuthMenu;
                    }
                }
                else if (screen == Screen::Login) {
                    if (e.key.code == sf::Keyboard::Tab) form.editLogin = !form.editLogin;
                    if (e.key.code == sf::Keyboard::Enter) {
                        if (sprawdzLogowanie(form.login, form.pass)) {
                            info = "";
                            rok = 1;
                            player = Player{};
                            inverted = false; invertedTurns = 0;
                            start = clock();
                            summaryTimeCaptured = false;

                            startLevel();
                            screen = Screen::Game;
                        }
                        else {
                            info = "Bledny login lub haslo.";
                        }
                    }
                }
                else if (screen == Screen::Game) {
                    if (e.key.code == sf::Keyboard::Up) movePlayer(0, -1);
                    else if (e.key.code == sf::Keyboard::Down) movePlayer(0, 1);
                    else if (e.key.code == sf::Keyboard::Left) movePlayer(-1, 0);
                    else if (e.key.code == sf::Keyboard::Right) movePlayer(1, 0);

                    if (player.knowledge <= 0 || player.energia <= 0) {
                        info = (player.knowledge <= 0 ? "Brak wiedzy! Powtarzasz rok." : "Brak energii! Powtarzasz rok.");
                        startLevel();
                    }

                    if (levelCompleted) {
                        if (rok == 3 || rok == 5) {
                            exam = makeExam(rok);
                            screen = Screen::Exam;
                        }
                        else {
                            screen = Screen::EndYear;
                        }
                    }
                }
                else if (screen == Screen::Trap) {
                    if (waitingTalisman) {
                        if (e.key.code == sf::Keyboard::T) {
                            player.talizman--;
                            waitingTalisman = false;
                            screen = Screen::Game;
                        }
                        if (e.key.code == sf::Keyboard::N) {
                            waitingTalisman = false;
                            applyTrap(trap.type);

                            if (returnToStart) {
                                maze[player.y][player.x] = ' ';
                                player.x = 1; player.y = 1;
                                returnToStart = false;
                                maze[player.y][player.x] = '@';
                            }

                            screen = Screen::Game;
                        }
                    }
                    else {
                        if (e.key.code == sf::Keyboard::Enter) {
                            applyTrap(trap.type);

                            if (returnToStart) {
                                maze[player.y][player.x] = ' ';
                                player.x = 1; player.y = 1;
                                returnToStart = false;
                                maze[player.y][player.x] = '@';
                            }

                            screen = Screen::Game;
                        }
                    }
                }
                else if (screen == Screen::EndYear) {
                    if (e.key.code == sf::Keyboard::Num1) endYearChoice = 1;
                    if (e.key.code == sf::Keyboard::Num2) endYearChoice = 2;
                    if (e.key.code == sf::Keyboard::Num3) endYearChoice = 3;

                    if (endYearChoice != 0) {
                        if (endYearChoice == 1) player.knowledge += 20;
                        if (endYearChoice == 2) player.energia += 1;
                        if (endYearChoice == 3) player.talizman += 1;

                        endYearChoice = 0;
                        rok++;
                        if (rok <= 5) {
                            startLevel();
                            screen = Screen::Game;
                        }
                        else {
                            if (!summaryTimeCaptured) {
                                clock_t stop = clock();
                                double czas = (double)(stop - start) / CLOCKS_PER_SEC;
                                int totalSeconds = (int)czas;
                                summaryMinutes = totalSeconds / 60;
                                summarySeconds = totalSeconds % 60;
                                summaryTimeCaptured = true;
                            }
                            screen = Screen::Summary;
                        }
                    }
                }
                else if (screen == Screen::Exam) {
                    if (e.key.code == sf::Keyboard::Enter) {
                        string ans = exam.input;
                        exam.input.clear();

                        if (ans != exam.qs[exam.idx].second) {
                            player.knowledge -= 30;
                            levelCompleted = false;
                            startLevel();
                            screen = Screen::Game;
                        }
                        else {
                            exam.idx++;
                            if (exam.idx >= 3) {
                                examResultMsg = "Gratulacje! Zaliczyles egzamin!\n\n";
                                if (exam.year == 3) {
                                    examResultMsg += "OBRONILES INZYNIERA!\n";
                                    examResultMsg += "NAGRODA: +50 WIEDZY\n";
                                    examResultMsg += "ODBLOKOWALES EVENT - MAGISTERKA!\n\n";
                                    player.knowledge += 50;
                                }
                                else if (exam.year == 5) {
                                    examResultMsg += "OBRONILES MAGISTRA!\n\n";
                                }
                                examResultMsg += "ENTER - kontynuuj";
                                screen = Screen::ExamResult;
                            }
                        }
                    }
                }
                else if (screen == Screen::ExamResult) {
                    if (e.key.code == sf::Keyboard::Enter) {
                        screen = Screen::EndYear;
                    }
                }
                else if (screen == Screen::Summary) {
                    if (e.key.code == sf::Keyboard::Enter) window.close();
                }
            }
        }

        if (screen == Screen::Game && steppedOnTrap) {
            steppedOnTrap = false;
            trap = makeTrap();
            waitingTalisman = trap.askTalisman;
            screen = Screen::Trap;
        }

        auto drawCentered = [&](const string& s) {
            window.clear(UI_BG);
            applyTextStyle(t, window);
            t.setString(s);
            centerText(t, window, 0.f);
            window.draw(t);
            window.display();
            };

        if (screen == Screen::StartSplash) {
            drawCover(window, assets.tStart);
        }
        else if (screen == Screen::AuthMenu) {
            drawCover(window, assets.tMenu);
        }
        else if (screen == Screen::Register || screen == Screen::Login) {
            string title = (screen == Screen::Register) ? "REJESTRACJA" : "LOGOWANIE";
            string s =
                title + "\n\n" +
                string(form.editLogin ? "> " : "  ") + "Login: " + form.login + "\n" +
                string(!form.editLogin ? "> " : "  ") + "Haslo: " + string(form.pass.size(), '*') + "\n\n" +
                "TAB - zmiana pola\n"
                "ENTER - zatwierdz\n\n";
            if (!info.empty()) s += info + "\n";
            drawCentered(s);
        }
        else if (screen == Screen::Game) {
            drawGame(window, assets);
        }
        else if (screen == Screen::Trap) {
            string s =
                "!!! WPADLES W PULAPKE !!!\n\n" +
                trap.name + "\n\n";
            if (waitingTalisman) {
                s += "Masz talizmanow: " + to_string(player.talizman) + "\n";
                s += "T - uzyj talizman (anuluj)\n";
                s += "N - nie uzywaj\n";
            }
            else {
                s += "ENTER - kontynuuj\n";
            }
            drawCentered(s);
        }
        else if (screen == Screen::EndYear) {
            string s =
                " KONIEC ROKU " + to_string(rok) + "\n"
                "Wybierz nagrode:\n"
                "1. Korepetycje (+20 do wiedzy)\n"
                "2. Napoj energetyczny (+1 do energii)\n"
                "3. Talizman szczescia (anuluje pulapke)\n";
            drawCentered(s);
        }
        else if (screen == Screen::Exam) {
            string s =
                " EGZAMIN KONCOWY ROKU " + to_string(exam.year) + " =\n"
                "Odpowiedz poprawnie na 3 pytania.\n\n" +
                string("Pytanie ") + to_string(exam.idx + 1) + ": " + exam.qs[exam.idx].first + "\n\n" +
                "Odp: " + exam.input + "\n\n"
                "ENTER - zatwierdz";
            drawCentered(s);
        }
        else if (screen == Screen::ExamResult) {
            drawCentered(examResultMsg);
        }
        else if (screen == Screen::Summary) {
            string s =
                "GRATULACJE! UKONCZYLES STUDIA!\n\n"
                "KARTA PRZEBIEGU STUDIOW\n\n"
                "STATUS: ABSOLWENT (Magister Inzynier)\n\n"
                "Laczna liczba ECTS: " + to_string(player.totalECTS) + "\n"
                "Koncowa wiedza:     " + to_string(player.knowledge) + "\n"
                "Koncowa energia:    " + to_string(player.energia) + "\n"
                "Czas trwania:       " + to_string(summaryMinutes) + "min " + to_string(summarySeconds) + "sek\n\n"
                "ENTER - zakoncz";
            drawCentered(s);
        }
    }

    return 0;
}

