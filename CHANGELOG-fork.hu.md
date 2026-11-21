# A magyar változat változásai

[English](CHANGELOG-fork.md) | **Magyar**

Ennek a változatnak (`Viktor318/zxtune`, `hungarian-ui` ág) a módosításai
az eredeti ZXTune `develop` ágához képest
([vitamin-caig/zxtune](https://github.com/vitamin-caig/zxtune)). Magának a
ZXTune-nak a változásai az eredeti repóban vannak.

## r5112-hu.1 – 2026-11-18

### Magyar felület

- **Teljes magyar fordítás**: a Qt felület (`l10n/hu/*.ts`), az üzenetek
  (`l10n/hu/*.po`) és a Qt saját párbeszédablakai (`qt.ts`, a Qt hivatalos
  magyar fordítása). Alapból a rendszer nyelve szerint indul; a nyelv a
  *Beállítások > Felület* lapon választható.
- A szóhasználat a ZX-forkok közös szójegyzékét követi (pl. Leállítás,
  Némítás, Ismétlés, Bővítmények, Hangkimenetek).
- A *Súgó > Névjegy* és a `zxtune123 --version` a kiadás verzióját mutatja
  (`r5112-hu.1`).

### TurboSound

- **3 chipes TurboSound** (9 csatorna, `03TS` lábléc, ZX Spectrum Next /
  MXTracker): eddig három külön PT3-ként jelent meg.
- **Chipenkénti keverés**: minden AY chipnek saját 3 csatornás keverője
  van, a chipek kimenete a chipszámmal osztva adódik össze.

### Keverő

- **AY-keverő** és **DAC-keverő** lap a *Beállításokban*. Az AY-keverő
  9 sora (AY 1–3, chipenként 3) minden AY/YM dalra hat (egy chip: AY 1,
  TurboSound: AY 1–2 vagy 1–3); a DAC-keverő a 3 csatornás (DMM, DST, STR)
  és a 4 csatornás (PDT, CHI, SQD, ET1) digitális dalokra.
- Csatornánként **hangerő** és **panoráma** (a Fuse és a ZEsarUX keverőjéhez
  hasonlóan); dupla kattintás a csúszkán: alapérték.
- **Állapotsor**: melyik dal szól, és a lap mely része hat rá; a szóló dal
  által használt chipek kiemelve.
- **Sztereó mód** az AY-keverőn (ugyanaz, mint az eszköztáron); a sorok
  neve a módot követi (pl. ACB: A1, C1, B1).
- **Csatornanémítás** a keverőn is (jelölőnégyzet a csatornanév előtt),
  szinkronban az eszköztár kék gombjaival; **Összes némítása** és
  **Összes engedélyezése** gomb; a *Minden csúszka alaphelyzetbe* csak a
  hangerőt és a panorámát állítja vissza.
- *Lejátszás > Keverő…* menüpont és **Keverő** gomb a hangerő-szabályzó
  mellett: a szóló dalhoz tartozó keverőlapot nyitja meg.
- A *Beállítások* ablak nem modális: mellette lehet dalt váltani. Ha
  kilépéskor nyitva volt, a következő indításkor ugyanott és ugyanazon a
  lapon nyílik meg.
- A főablak legkisebb szélessége a csatornagombokhoz igazodik (a
  hangerő-résznek legalább 300 képpont marad).

### Hang

- A 3 csatornás keverők **középső csatornájának alapértéke 71%** (60%
  helyett, kb. -3 dB, egyenletes panoráma). A már elmentett értékek
  megmaradnak.
- Javítva: a **CAB és CBA sztereó mód** fel volt cserélve.
- Javítva: a **némított csatorna** nem volt néma, csak a hangja tűnt el;
  a hangerő-változások kattogásként hallatszottak. Most teljesen elhallgat.
  Az **N** (zaj) némítása a csak zajt játszó csatornákat is elnémítja.
- Javítva: az **E** (burkológörbe) némítása minden csatornát teljes
  hangerőre állított; most a burkológörbét használó csatornák hallgatnak el.

### Lejátszás

- Javítva: a **tekerés** (a pozíciócsúszka) leállította a lejátszást,
  néha „std::exception” hibával – az AY-alapú, az `.ay` és a DAC dalokban.

### Az eredeti ZXTune-ból behozva

- FastTracker, ASC Sound Master, ProTracker 2 és 3, Sound Tracker Pro,
  ProDigiTracker és SQ Digital Tracker lejátszási javítások, AHX/HVL
  felismerés (2026-11-10 – 11-17).
