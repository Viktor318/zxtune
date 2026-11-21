# ZXTune – magyar kiadás

[English](README-fork.md) | **Magyar**

Ez a [ZXTune](https://github.com/vitamin-caig/zxtune) chiptune-lejátszó egy
változata Windowsra, magyar felülettel. A ZXTune a ZX Spectrum és sok más
régi gép zenéit játssza le (PT3, STC, ASC, TurboSound, AY, DAC, SID, NSF…).
Ez a kiadás a ZXTune `develop` ágára épül (az r5112 utáni, 2026. november
17-i állapot), és magyar felülettel, 3 chipes TurboSounddal, új keverővel
és néhány hibajavítással egészíti ki.

Az eredeti ZXTune leírása a [README.md](README.md) fájlban van (angolul). Ennek
a változatnak a módosításai a [CHANGELOG-fork.hu.md](CHANGELOG-fork.hu.md)
fájlban olvashatók.

## Letöltés

A [Releases](https://github.com/Viktor318/zxtune/releases) oldalról töltsd le
a `ZXTune-r5112-hu.1-windows.zip` fájlt, csomagold ki egy tetszőleges
mappába, és indítsd el a `zxtune-qt.exe`-t. Telepíteni nem kell semmit, a
program egyetlen fájl, minden benne van.

- `zxtune-qt.exe`: a grafikus lejátszó.
- `zxtune123.exe`: parancssori lejátszó és konvertáló (pl. `zxtune123 --wav
  filename=dal.wav dal.pt3`).

## Mit ad hozzá ez a kiadás

### Magyar felület
- Minden menü, párbeszédablak és üzenet magyarul, a Qt saját ablakaival
  együtt. Alapból a Windows nyelvét követi; a nyelv a *Beállítások > Felület*
  lapon választható.

### TurboSound
- **3 chipes TurboSound** (9 csatorna, ZX Spectrum Next / MXTracker, `03TS`)
  és a 2 chipes `.ts` dalok chipenkénti keverése.

### Keverő
- **AY-keverő** és **DAC-keverő** (*Lejátszás > Keverő…*, vagy a **Keverő**
  gomb a hangerő mellett): csatornánként hangerő és panoráma, sztereó mód,
  csatornanémítás (szinkronban az eszköztár kék gombjaival), *Összes
  némítása*, *Összes engedélyezése*, *Minden csúszka alaphelyzetbe*. Az
  állapotsor megmutatja, melyik dal szól, és a lap mely része hat rá.
- A *Beállítások* ablak nyitva maradhat lejátszás közben; ha kilépéskor
  nyitva volt, a következő indításkor ugyanott nyílik meg.

### Hang és lejátszás
- A középső csatorna alapértéke 71% (60% helyett).
- Javítva: a tekerés (pozíciócsúszka) megállította a lejátszást; a némított
  csatorna kattogott; az E (burkológörbe) némítása mindent felhangosított;
  a CAB és CBA sztereó mód fel volt cserélve.

## Beállítások

A program a beállításait ugyanott tartja, mint az eredeti ZXTune (a Windows
regisztrációs adatbázisában), így egy korábbi ZXTune beállításait is
használja.

## Forráskód és fordítás

Forráskód: a [`hungarian-ui`](https://github.com/Viktor318/zxtune/tree/hungarian-ui)
ág. A Windowsos programot Linuxon (WSL2) keresztfordítjuk llvm-mingw-vel,
statikus Qt 5.15.2-vel és boost 1.82-vel, ahogy az eredeti ZXTune Windowsos
kiadásait.

## Licenc

A ZXTune licence alatt (lásd [LICENSE.md](LICENSE.md)).
