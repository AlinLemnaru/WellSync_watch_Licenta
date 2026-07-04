# WellSync Watch – Proiect de licență

## Descriere generală

Acest document conține livrabilele proiectului de licență **WellSync**, având ca rezultat realizarea unui prototip de smartwatch pentru monitorizarea sănătății, bazat pe platforma **M5Stack Core2**. Aplicația embedded dezvoltată integrează senzori pentru monitorizarea pulsului și a saturației de oxigen din sânge, a parametrilor de mediu, a numărului de pași și a evenimentelor de cădere, precum și o interfață grafică locală și comunicație Bluetooth Low Energy cu o aplicație companion.

Repository-ul include **întregul cod sursă al proiectului**, fără fișiere binare compilate.

## Livrabilele proiectului

Repository-ul include următoarele livrabile:

- codul sursă complet al aplicației embedded pentru smartwatch;
- module software pentru:
  - monitorizarea pulsului și SpO2 folosind senzorul MAX30100;
  - monitorizarea temperaturii, umidității, presiunii și calității aerului folosind senzorul BME680;
  - numărarea pașilor și detecția căderilor folosind MPU6886;
  - afișarea datelor pe interfața grafică a dispozitivului;
  - gestionarea funcțiilor de alarmă, cronometru, temporizator și standby;
  - comunicarea BLE cu aplicația de pe telefon;
- fișierele de configurare și structură necesare recompilării proiectului.

**Notă:** Repository-ul nu conține fișiere compilate sau artefacte generate la build, precum `.bin`, `.elf`, `.hex` sau alte fișiere binare similare.

## Cerințe hardware

Pentru compilare, încărcare și rulare sunt necesare următoarele componente hardware:

- M5Stack Core2;
- senzor MAX30100;
- senzor BME680;
- conexiunile hardware corespunzătoare între module;
- cablu USB pentru programare;
- sursă de alimentare / baterie pentru utilizarea prototipului.

## Cerințe software

Pentru compilarea aplicației este necesar:

- **Arduino IDE**;
- suport pentru plăci **ESP32 / M5Stack Core2** instalat în Board Manager;
- bibliotecile necesare proiectului, printre care:
  - `M5Unified`
  - `M5GFX`
  - `Wire`
  - `Adafruit Unified Sensor`
  - `Adafruit BME680`
  - celelalte biblioteci utilizate de codul sursă.

## Pașii de compilare ai aplicației

1. Clonarea repository-ului:
   ```bash
   git clone https://github.com/AlinLemnaru/WellSync_watch_Licenta.git
   ```

2. Deschiderea proiectului în Arduino IDE.

3. Deschiderea fișierului principal al aplicației:
   - `SmartWatchApp.ino`

4. Instalarea tuturor bibliotecilor necesare din Arduino Library Manager.

5. Selectarea plăcii corecte din meniul:
   - `Tools > Board > M5Stack Core2` (sau varianta echivalentă disponibilă în mediul instalat)

6. Selectarea portului serial corect din:
   - `Tools > Port`

7. Compilarea aplicației prin apăsarea butonului:
   - **Verify / Compile**

## Pașii de instalare a aplicației pe dispozitiv

1. Conectarea dispozitivului M5Stack Core2 la calculator prin cablu USB.
2. Verificarea selectării portului serial corect în Arduino IDE.
3. Apăsarea butonului:
   - **Upload**
4. Așteptarea finalizării procesului de compilare și încărcare pe dispozitiv.
5. După finalizarea upload-ului, dispozitivul va reporni automat.

## Pașii de lansare a aplicației

După încărcarea firmware-ului pe dispozitiv, aplicația pornește automat la boot, fără a fi necesare comenzi suplimentare de lansare.

La pornire:
1. este afișat ecranul de tip splash;
2. aplicația încarcă serviciile și modulele necesare;
3. este afișată interfața principală a smartwatch-ului, din care utilizatorul poate naviga către paginile disponibile.

## Observații

- Proiectul este realizat în scop academic, în cadrul lucrării de licență.
- Pentru rularea corectă a aplicației este necesară conectarea senzorilor conform configurației hardware utilizate în proiect.
- Se recomandă păstrarea unei structuri nemodificate a fișierelor sursă pentru a evita erori de compilare.
- Fișierele binare generate la compilare nu trebuie adăugate în repository.
