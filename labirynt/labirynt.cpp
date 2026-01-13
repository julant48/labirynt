string szyfrowanie(string tekst) {//funkcja szyfrująca nazwe i hasło
    int n = 4;
    for (int i = 0;i < tekst.length();i++) {
        tekst[i] = tekst[i] + n;
    }
    return tekst;
}

void rejestracja(string nazwa, string haslo) {//funkcja pierwszej rejestracji
    cout << "Nowy login: ";
    cin >> nazwa;
    cout << "Nowe haslo: ";
    cin >> haslo;

    nazwa=szyfrowanie(nazwa);//szyfrowanie nazwy
    haslo=szyfrowanie(haslo);
    
    fstream plik;
    plik.open("dane.txt", ios::out);
    plik << nazwa << endl;//zapisanie zaszyfrowanej nazwy do pliku
    plik << haslo << endl;
    plik.close();
    cout << "Zarejestrowano";
  }
void logowanie(string nazwa, string haslo) {//funkcja logowania
    cout << "Podaj login: ";
    cin >> nazwa;
    cout << "Podaj haslo: ";
    cin >> haslo;

    nazwa = szyfrowanie(nazwa);//szyfrowanie nazwy
    haslo = szyfrowanie(haslo);
    fstream plik("dane.txt");
    string linia_1;
    string linia_2;
    getline(plik, linia_1);//odczytanie pierwszej lini w pliku, którą jest nazwa
    getline(plik, linia_2);//odczytanie drugiej lini w pliku, którą jest hasło
    if (nazwa == linia_1 && haslo == linia_2) {//sprawdza czy hasło i nazwa sa poprawne
        cout << "Zalogowano" << endl;
}else {
        cout << "Bledny login lub haslo." << endl;
    }
}

int main() {
    int wybór;
    cout <<"  MAZE RUNNER" << endl;
    cout << "1. Resjestracja" << endl;
    cout << "2. Logowanie " << endl;
    cout << "Wybierz numer: " << endl;
    cin >> wybór;
    string nazwa;
    string haslo;
    switch (wybór) {
    case 1: {
        rejestracja( nazwa, haslo);
        break;
    }
    case 2: {
        logowanie(nazwa, haslo);
        break;
    }
    }
    return 0;
}
