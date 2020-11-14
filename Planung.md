# Anforderungen
* Ziel ist portables Gerät zur Smarthome-Bedienung im ausgefallenen Gehäuse

* Gerät ist Batteriebetrieben
* Gerät hat Funkanbindung
* Gerät geht nach einiger Zeit in Standby
* Beliebiger Tastendruck weckt Gerät auf
* In Standby so wenig Energieverbrauch wie möglich

# Konzept
* Funkkommunikation geschieht über WLAN => flexibelste Lösung
* Gerät wird mit Lithium-Akku betrieben, Ladeschaltung inkludiert
* Ladeadapter über Klinkenstecker
* Für WLAN wird ein ESP8266 verwendet
* Für Auswertung der Tastatur wird ein Atmega88 als Hilfsprozessor verwendet (dieser kann sehr Energiesparsam programmiert werden)
* Kommunikation über I2C, wobei Hilfsprozessor mit Akkuspannung läuft, ESP hat eigenen LDO und I2C Level-Shifter

# Umsetzung

## AVR-Board: Atmega88 auf Universalboard 0.9 (Karsten)
Folgende Pins stehen zur Verfügung:

* PC1 - Zeile 1/2
* PC2 - Zeile 3
* PC3 - Zeile 4

* PD0 - Spalte A
* PD1 - Spalte B
* PD2 - Spalte C
* PD3 - Spalte D
* PD4 - LED Reihe 1
* PD5 - LED Reihe 2
* PD6 - Lautsprecher in

* PB0 - Wakeup ESP?

### PIN-Matrix
1. Zeile 4
2. Zeile 1/2
3. Spalte D
4. Zeile 2
5. Spalte C
6. Röntgen1
7. Spalte B
8. Röntgen2
9. Spalte A

