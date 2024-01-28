<img src="./images/put_logo_text.png" alt="Logo of the project" align="right">

# Continuous directory synchronization. Project for SK2
> Additional information

A program that ensures that the content of the catalog is the same for all clients. File changes are detected immediately.

*Action*: After starting and providing the path to the folder to be synchronized, the client can add, modify and delete files from it. The server monitors these changes and sends them to other connected clients

*Required*: Concurrent directory change can be detected and handled properly. One server that knows about other agents.

___

### Opis Server.cpp

Program rozpoczyna się od odczytu konfiguracji z pliku podanego w argumentach wiersza poleceń.

*Odbieranie ścieżki od klienta:*

Serwer odbiera od klienta ścieżkę do folderu synchronizacji (recv). Ta ścieżka jest używana w funkcji synchronizeFolders do określenia ścieżek folderów klienta i serwera do synchronizacji.

*Synchronizacja plików:*

Wywoływana jest funkcja synchronizeFolders, która porównuje pliki w folderze klienta z plikami na serwerze i vice versa. Dla każdego pliku porównywane są czasy ostatniej modyfikacji, i jeśli plik się różni, jest kopiowany z jednego komputera na drugi.

*Wysyłanie odpowiedzi klientowi:*

Po zakończeniu synchronizacji serwer wysyła klientowi wiadomość "Bidirectional Synchronization complete" (send).

---

### Opis Client.py

Kod klienta również zaczyna się od odczytu konfiguracji z pliku podanego w argumentach wiersza poleceń.

*Wysyłanie ścieżki folderu do serwera:*

Klient wysyła do serwera ścieżkę do swojego folderu synchronizacji (config.folder_path.encode(), send).

*Wysyłanie czasów ostatniej modyfikacji plików:*

Klient wysyła serwerowi czasy ostatniej modyfikacji plików w swoim folderze.
Dla każdego pliku używane jest struct.pack('!Q', last_modified) do spakowania czasu do bajtów i send do wysłania.

*Odbieranie odpowiedzi od serwera:*

Klient odbiera odpowiedź od serwera po zakończeniu synchronizacji (client_socket.recv(1024)).

*Odpowiedź jest wyświetlana na ekranie.*

---

### Configure program

Najpierw w plikach konfiguracyjnych *"config.json"* trzeba zmienić ścieżki folderów na swoje (znajdują się te pliki w folderach *server* oraz *client*)

```
mkdir build
cd build
cmake ..
make
```
Po tym pojawi się folder *build* z potrzebującymi plikami dla działania programu.

---

### Run program

**For run Server.cpp**

```
cd build
./server server_config.json
```

**For run CLient.py**
```
cd Client
python3 client.py config.json
```

## Developers
**Heorhi Zakharkevich - [153992](https://usosweb.put.poznan.pl/kontroler.php?_action=katalog2/osoby/pokazOsobe&os_id=107761)**

**Rastsislau Chapeha - [153996](https://usosweb.put.poznan.pl/kontroler.php?_action=katalog2/osoby/pokazOsobe&os_id=107765)**

---

### Dziłanie programu

![](https://github.com/Irrisorr/folder_sync/blob/main/images/gif.gif)
