# Mokosh

Szablon firmware urządzenia dla systemu Mokosh.

Wykorzystaj jako podstawę do budowania swojego projektu zgodnie z regułami,
które ustaliliśmy do tej pory.

## Budowanie

Aby zbudować niezbędne jest przygotowanie pliku `BuildVersion.h`, na podstawie
szablonu `BuildVersion.template.h`. Skrypt `buildversion.cmd` automatycznie
przygotuje taki plik, o ile posiada się `GitVersion.exe` oraz `sed`. Plik
`BuildVersion.h` zawiera informacje o wersji konkretnego builda (SemVer jako
wersja główną, którą się firmware przedstawia, oraz InformationalVersion, którą
można wymusić odpowiednią komendą).

Aby zbudować niezbędna jest również konfiguracja buildu, plik `BuildConfig.h`,
na podstawie szablonu `BuildConfig.template.h`. Konfiguracja buildu ustawia:

* flagi włączania funkcji firmware (`ENABLE_RAND` oraz `ENABLE_NEOPIXEL`),
* prefiks ciągu identyfikacyjnego urządzenia (np. `MOKOSH`),
* piny, liczbę LED-ów i tym podobne,
* poziom "gadatliwości" na wyjściu szeregowym,
* ścieżki do pliku aktualizacji na serwerze aktualizacji.

### Skrypty

Aby zbudować projekt w trybie CLI można skorzystać ze skryptu `build.cmd`, który
wykorzystuje `arduino-cli` do budowania projektu.

Skrypt `flash.cmd` uruchamia flashowanie zbudowanego projektu poprzez
`arduino-cli`, użycie: `flash.cmd <port> <plik>`.

Skrypt `clean.cmd` czyści katalog ze zbudowanymi binarkami.

Skrypty `publish-bin.cmd` oraz `private-bin.cmd` kopiują zbudowany projekt i go
odpowiednio oznaczają, tagują i ewentualnie wyliczają sumę kontrolną.

## Debug Level

Poziom "gadatliwości" na wyjściu szeregowym (domyślnie 115200 kbps) jest
określany przz stałą `DEBUG_LEVEL`, która przyjmuje wartość 1, 2, 4 lub 8. Kiedy
poziom ustawiony jest na określoną liczbę, wszystkie komunikaty z poziomu
równego lub większego są przesyłane na port szeregowy, stąd `DEBUG_LEVEL 2`
prześle komunikaty z poziomów `DLVL_INFO` (2), `DLVL_WARNING` (4) oraz
`DLVL_ERROR` (8), ale nie `DLVL_DEBUG` (1). Buildy z ustawionym poziomem 4 lub 8
są określane jako "release".

## SPIFFS

Konfiguracja dla modułu zapisana jest w SPIFFS. Do katalogu `data/` należy
skopiować plik `config.json`, na podstawie `config.template.json`, zawierający
konfigurację WiFi, brokera, serwera aktualizacji i dodatkowe ustawienia. Potem
należy sflashować SPIFFS poprzez [ESP8266FS dla
Arduino](https://github.com/esp8266/arduino-esp8266fs-plugin/releases).

Alternatywnie można użyć `esptool` oraz `mkspiffs` w sposób pokazany w skrypcie
`spiffs.cmd`, który wysyła zawartość katalogu `data/`.

Alternatywnie można skorzystać z przesyłania plików podczas pierwszej
konfiguracji urządzenia.

## MQTT

Zawiera mechanizmy komunikacji z brokerem MQTT. Domyślnie dane wysyłane są z
topic postaci `MOKOSH_XXXXX/<topic>`, gdzie XXXXX oznacza automatycznie
generowany identyfikator urządzenia, opierający się o jego numer seryjny.

Subtopici:

* `/version` -> urządzenie zgłasza tutaj po uruchomieniu lub po wydaniu komendy,
  swoją wersję oprogramowania (pakiet "hello"). Pakiet ma postać SEMVER, tj.
  `MAJOR.MINOR.PATCH`, np. `0.7.1`,
* `/rand` -> urządzenie zgłasza tutaj zmianę wartości wirtualnego czujnika (nowa
  wartość),
* `/debug/heartbeat` -> zgłoszenie stanu życia (według konfiguracji z pliku
  `BuildConfig.h`, domyślnie co 10 sekund, wewnętrzny zegar czasu uruchomienia),
* `/debug` -> dodatkowe informacje diagnostyczne,
* kolejne czujniki powinny zgłaszać swoje nowe wartości na swoich własnych
  topicach, np. `/temperature`, `/weight` itp.

Urządzenie natomiast subskrybuje topic `/cmd`, który pozwala na wysyłanie do
urządzenia komend:

* `getr` -> powoduje wysłanie pakietu z aktualną wartością wirtualnego czujnika
  do `/rand` (tylko dla aktywnego `ENABLE_RAND`),
* `led1` -> włączenie wbudowanej diody LED lub zaświecenie paska NeoPixel,
  zgodnie z konfiguracją koloru danego urządzenia (tylko dla aktywnego
  NeoPixel),
* `led0` -> wyłączenie wbudowanej diody LED lub paska NeoPixel,
* `gver` -> ponowne wysłanie identyfikatora wersji do `/version`,
* `getfullver` -> wysłanie do `/debug` pełnego identyfikatora wersji (tj.
  `InformationalVersion`, np. `0.7.1+2.Branch.master.Sha.<sha commitu>`), a
  potem flag budowania, np. `RELEASE+RAND+NEOPIXEL`,
* `gmd5` -> wysłanie do `/debug` sumy MD5 aktualnego firmware,
* `reboot` -> ponowne uruchomienie urządzenia,
* `showerror=xx` -> przejście w tryb błędu z kodem xx (numerycznie, np. 500),
* `setcolor=rrr,ggg,bbb` -> ustawienie koloru wagi na kolor RRR,GGG,BBB,
  wymagany jest zawsze format trzycyfrowy (tylko dla NeoPixel),
* `showcolor=rrr,ggg,bbb` -> zaświecenie paska NeoPixel na kolor RRR,GGG,BBB
  (tylko dla NeoPixel),
* `sota=<wersja>` -> przejście w tryb aktualizacji OTA do wersji przekazanej w
  parametrze,
* `factory` -> usunięcie pliku `config.json` ze SPIFFS i ponowne uruchomienie
  urządzenia (reset do ustawień fabrycznych),
* `printconfigserial` -> wypisanie na port szeregowy aktualnej konfiguracji w
  postaci JSON,
* `setconfig=<field>,<value>` -> aktualizacja konfiguracji - pola `field` do
  wartości `value`,
* `updateconfig` -> zapis aktualnej konfiguracji do pliku,
* `reloadconfig` -> wymuszenie przeładowania konfiguracji z pliku `config.json`.

## Logika pracy firmware

### Uruchamianie

W przypadku niepowodzeń części z operacji (połączenie z Wi-Fi itp.) firmware
przechodzi do trybu wyświetlania błędu, a po pewnym czasie się zrestartuje
i spróbuje ponownie.

1. Nawiązywane jest połączenie przez port szeregowy i ustawiany poziom
   gadatliwości,
2. Uruchamiany jest pasek NeoPixel i animacja początkowa (tylko dla NeoPixel),
3. Konfigurowany jest sposób prezentowania błędów,
4. Pierwsza dioda od lewej zaświeca się na zielono,
5. Następuje odczyt konfiguracji,
6. Druga dioda zaświeca się na zielono,
7. Następuje połączenie z siecią Wi-Fi,
8. Trzecia dioda zaświeca się na zielono,
9. Następuje połączenie z brokerem MQTT,
10. Czwarta i piąta dioda zaświecają się na zielono,
11. Następuje publikacja wersji (pakiet Hello),
12. Szósta dioda zaświeca się na zielono,
13. Następuje publikacja wartości czujnika RAND,
14. Pasek LED-owy się wyłącza.

### Tryb normalnej pracy

1. Jeśli nie ma połączenia z MQTT, następuje próba ponownego połączenia,
2. Jeśli została wysłana komenda MQTT, następuje wysłanie odpowiedzi,
3. Jeśli czas przekracza czas od ostatniego wysłania komunikatu heartbeat,
   wysyłany jest heartbeat,
4. Jeżeli wartość czujnika przekracza zdefiniowaną róznicę, wysyłany jest
   komunikat z nową wartością,
5. Urządzenie pauzuje się na 100 milisekund i kontynuuje pętlę.

### Tryb first-run (pierwsza konfiguracja)

1. Jeśli plik konfiguracyjny `config.json` nie istnieje, urządzenie wchodzi w
   tryb pierwszej konfiguracji.
2. Tworzy otwartą sieć Wi-Fi o nazwie MOKOSH_XXXXX,
3. Uruchamia serwer HTTP pod adresem 192.168.4.1,
4. Rozpoczyna oczekiwanie na połączenie i ich obsługę.
