# ICP projekt
Autoři: David Brož & Kristián Jacik

# Grafická aplikace
V grafické části hry je jedno prostředí ohraničené zídkou, po kterém se může uživatel pohybovat. Uživatele pronásleduje míč, který po kontaktu s kamerou spustí audio, které signaluje zásah.


Ovládání
- Klávesy W,S,A,D pro pohyb
- Myš pro rozhlížení
- Kolečko myši mení FOV
- F pro změnu z okna na fullscreen
- ESC pro ukočení programu

# Sestavení
Pro sestavení projektů je potřeba mít vytrovřenou systémovou proměnou OPENCV_DIR která směřuje do složky /build opencv. A do PATH přidat %OPENCV_DIR%\x64\vc15\bin. Zbylé závislosti už jsou řešeny v rámci projektu a jsou součástí repozitáře. (glew, glfw...)
