Eraldusvõime määramine praktikas
Ma olen eksinud. Ma andsin ülesandeks määrata süsteemi eraldusvõime. Ma sain tagasi väga leidlikke ai maitselist akrobaatikat, kus sama asja arvutati kolmel erineval moel kasutades anduri spetsifikatsiooni ja ADC (A/D-muundur) bittide arvu. Ükski neist ei olnud päris resolutsioon/eraldusvõime - minu arust.
Tavaliselt võtame ADC väärtuse, korrutame läbi pingejaguri skaleerimise, kasutame tootja valemit - saame rõhu kPa-des. Tundub loogiline? On ka. Aga see ei ütle sulle süsteemi eraldusvõimet. Selle abil saab kontrollida kas valem on õigesti implementeeritud.
Kui tahad eraldusvõimet, pead leidma mõne teise loogika. Pead vaatama mida andur tegelikult teeb, mitte mida spetsifikatsioon lubab. Kui paned kolme erineva rõhupunkti paika ja mõõdad igaüht kümme korda, siis numbrid hüppavad. Mitu ADC ühikut nad hüppavad? See hüppamine ongi eraldusvõime - tiheduse piir millest täpsemini ei ole võimalik näha.
spetsifikatsioon ütleb et andur suudab mõõta 0-700 kPa. ADC on 12-bitine, seega 4096 võimalikku väärtust. Matemaatika ütleb: 700/4096 = 0.17 kPa per ADC ühik. Pühin tolmu pükstelt ja hakkan juba minema?
Oot. Toitepinge kõigub. Takistid pole päriselt 10kΩ vaid 9.8kΩ või 10.2kΩ. Kristall driftib temperatuuriga. PCB-l on jootekohad mis hakkavad oksüdeeruma. Anduri korpuses on niiskust. Kõik see kokku tekitab müra. Müra sööb eraldusvõime ära.
Mis asi on eraldusvõime siis päriselt
Eraldusvõime ei ole spetsifikatsiooni number. Eraldusvõime on kaks asja mida sa mõõdad:
Esiteks: mitu kPa on üks ADC ühik sinu süsteemis (mitte spetsifikatsiooni järgi, vaid päriselt).
Teiseks: mitu ADC ühikut hüppab signaal paigalseisu ajal.
Kui paned rõhu paika ja loed 100 mõõtmist, siis need ei ole kõik sama number. Need hüppavad. Näiteks: 290, 287, 293, 289, 291, 295, 288, 292... See hüppamine on müra.
Kui müra on ±3 ADC ühikut ja sinu teisendus on 0.17 kPa per ADC ühik, siis praktiline eraldusvõime on 3 × 0.17 = 0.5 kPa.
Kui müra on ±10 ADC ühikut, siis eraldusvõime on 10 × 0.17 = 1.7 kPa.
See on vahe teooria ja praktika vahel. spetsifikatsioon ütleb 0.17 kPa. Sinu mõõtmistulemused ütlevad 0.5 kuni 1.7 kPa.
Aga ära sega eraldusvõime ja täpsust omavahel. Eraldusvõime ütleb kui väikest muutust sa suudad eristada. Täpsus ütleb kui lähedal sa oled laua peal toimuvale reaalsusele. Need on kaks eri asja.
Eraldusvõime on süsteemi suhteline tihedus - kui peeneks sa suudad maailma viilutada. Täpsus on see, mitu viilu sul vaja on et võileib ei jääks poolpaljaks.
Kujuta ette et sa tead kindlat: üks võileib on neli viilu vorsti. Neli viilu. Mitte kolm, mitte viis.
Sa lähed ja viilutad vorsti. Sul tuleb seitse viilu. Seitse on hea resolutsioon - sa suutsid peenelt viilutada. Aga seitse ei jagu kahe võileiva peale (sest üks võileib on neli viilu). Üks võileib saab valmis, jääb kolm viilu järele. See ei ole täpne.
*isadepäeva seletus
MPX5700AP anduri täpsus on ±2.5% spetsifikatsiooni järgi. See tähendab et isegi kui su eraldusvõime on 0.5 kPa, võib tegelik rõhk olla veel ±2.5% jagu ebatäpsem.
Näide: sa mõõdad 100 kPa. Su eraldusvõime on 0.5 kPa - sa suudad eristada 99.5 kPa ja 100.5 kPa vahelist erinevust. Aga anduri täpsus on ±2.5 kPa. Seega tegelik rõhk võib olla 97.5 kuni 102.5 kPa vahel. Sa näed väikseid muutusi täpselt, aga absoluutne väärtus võib olla hoopis ebatäpsem.
Kuidas eraldusvõimet mõõta
Vahendid
Sa vajad:
●	N100 süstal (10ml)
●	Termorüüs et kinnitada toru süstlale ja andurile
●	4mm rõhutoru
●	4mm kiirühendused
●	MPX5700AP andur
●	M5 Atom või muu mikrokontroller
●	3D trükitud hoidikud kolmes suuruses (8ml, 5ml, 2ml märgistustega)
Hoidikud on selleks, et kui surud süstla kokku, siis kolb jääb täpselt õigesse kohta. Muidu libiseb käsi ja mõõtmine läheb katki.
Katse käik
1.	Tõmba süstla kolb täpselt 10ml joonele. 
2.	Ühenda kiirühendusega süstal anduriga. Kontrolli et ühendus on tihe. Kui õhk lekib, siis numbrid on mõttetud.
3.	Käivita salvestus. Seadista 5 mõõtmist sekundis (200ms vahega). Rohkem pole vaja - rõhk muutub aeglaselt.
4.	Suru süstal ettenähtud vahemikku. Kasuta 3D hoidikut - 8ml, 5ml või 2ml märgistusega. Hoidik fikseerib kovi täpseks mõõtmiseks.
5.	Hoia 10 sekundit paigal. Ära liiguta, ära vaata imelikult, ära hingagi. Lihtsalt oota.
6.	Lase kiirühendus lahti. Võta 3D hoidik ära. Lõpeta salvestus. (nüüd me saame eeldada et salvetus on selline alguse poolt terva kõrge äärega hammas)
7.	Korda sama protseduuri 10 korda iga rõhu taseme kohta. Kokku 30 mõõtmist.
Mida sa saad
Iga mõõtmise kohta on sul salvestus kus näed kuidas rõhk kasvab ja siis stabiliseerub.
Esimesed paar sekundit on kaos - kolb liigub, rõhk kasvab, numbrid hüppavad. Siis rõhk jõuab maksimumini ning langeb tagasi (kui kolb on lõpuks 3d trükitud hoidikus) ja hakkab stabiliseeruma. Ootad liiga kaua, süsteem lekib - rõhk langeb. Parim koht kus mõõta on kohe peale piiki kui rõhk on stabiliseerunud.
Võta analüüsiks 5 sekundit keskelt - pärast stabiliseerumist, aga enne kui hakkab langema. Leia kindlasti stabiliseerimise kohta mitte ära võta keskmist  Vaata graafikut ja leia see tasane osa. Kui võtad liiga algusest, siis seal on veel kasvu ja ülevise kuna süstal suruti hoidikusse. Kui võtad liiga hilja, siis seal on juba languse osa veast liiga suureks kasvanud.
Näide:
Salvestus 10 sekundi jooksul (5 Hz, 50 punkti):
Kasuta ainult stabiilset osa (X;(X+5)s). Keskmine: X ADC ühikut.
Tee see iga 10 mõõtmise kohta. Siis sul on 10 keskmist iga rõhu taseme kohta:
Näiteks 2ml kompressioon, 10 mõõtmise keskmised (stabiilne osa): [285, 292, 288, 295, 290, 287, 293, 289, 291, 294] 
*näiteks random numbrid ;) nii kaa edaspidi ärge neid palun alusenda või suuruse eeldusena kasutage
Keskmine: 290.4 ADC ühikut
Standardhälve: 3.1 ADC ühikut
See 3.1 on sinu praktiline müra tase. Vahemik mille sees ei saa kindel olla mis on siis tegelik mõõdetud sisendväärtus.
Standardhälve ≠ Müra amplituud
Miks need erinevad?
Standardhälve (σ):
●	Kirjeldab andmete hajuvust keskmise ümber
●	Ütleb: "Andmed on keskmiselt ±σ kaugusel keskmisest"
●	Ei anna otse müra maksimaalset amplituudi
Müra amplituud:
●	Müra maksimaalne võimalik hälve
●	Praktiline piir: "Müra ei ületa kunagi ±X ühikut"
Kordaja leidmine sõltumatult
Sina tead et 2ml, 5ml ja 8ml kompressioon on teadaolevad sammud. Need on mõlemat pidi 3ml samm.
Kui sul on kolm punkti ADC väärtustes:
●	8ml positsioon (2ml kompressioon): keskmine = 290 ADC
●	5ml positsioon (5ml kompressioon): keskmine = 671 ADC
●	2ml positsioon (8ml kompressioon): keskmine = 2204 ADC
Esimene samm (8ml → 5ml): 671 - 290 = 381 ADC ühikut 3ml kohta Teine samm (5ml → 2ml): 2204 - 671 = 1533 ADC ühikut 3ml kohta
Need pole võrdsed. Esimene on 127 ADC/ml, teine on 511 ADC/ml. Süsteem pole lineaarne.
spetsifikatsioon ei seleta kuidas kontrollida kas sinu analoog osa on lineaarne vaid eeldab et see on juba tehtud seega….
Lineaarsuse Test
Kui sammud pole võrdsed, siis rõhk ei kasva lineaarselt kompressiooniga.
Põhjused:
●	Surnud maht torudes
●	Õhuleke
●	Boyle'i seadus ei kehti täpselt (õhk pole ideaalne gaas suure rõhu juures)
●	Süstla geomeetria muutub
Aga sina ei tea veel mis põhjus on. Sa tead ainult: süsteem pole lineaarne. Üksi süsteem pole ideaalne. 
Linear regression annab sulle parima sirge läbi kõigi kolme punkti:
from scipy.stats import linregress
compression = [2, 5, 8]  # ml
adc_values = [290, 671, 2204]  # keskmised
slope, intercept, r_value, p_value, std_err = linregress(compression, adc_values)
R² väärtus ütleb kui hästi andmed sirgele sobivad. Kui r² < 0.99, siis midagi on valesti.
Praktiline eraldusvõime
Nüüd sul on:
●	slope (kordaja): mitu ADC ühikut per ml kompressiooni
●	müra: standardhälve ADC ühikutes igal rõhu tasemel
Eraldusvõime on: müra / slope
Kui slope = 300 ADC/ml ja müra = 3 ADC ühikut, siis:
●	Eraldusvõime = 3 / 300 = 0.01 ml ehk 10 mikroliitrit
See on see number mis  ütleb: kui väikest mahu muutust sa suudad eristada.
kPa teisendus
Tahad teada: mitu kPa on üks ADC ühik?
Selleks kasutame Boyle'i seadust kui alusprintsiipi. Boyle ütleb mida peaks saama ideaalse süsteemiga. Sinu mõõtmised näitavad mida tegelikult saad.
Boyle'i seadus õhu jaoks (gauge pressure):
●	2ml kompressioon: P = 101.3 × (10/8) - 101.3 = 25.3 kPa
●	5ml kompressioon: P = 101.3 × (10/5) - 101.3 = 101.3 kPa
●	8ml kompressioon: P = 101.3 × (10/2) - 101.3 = 405.2 kPa
Boyle'i seadus kehtib õhu jaoks üsna täpselt (±0.5%) kuni 5 bar. Kui sinu mõõtmised erinevad sellest rohkem, siis põhjus pole gaas - põhjus on mehaaniline süsteem.
Nüüd tee tabel:
Kompressioon	Teoreetiline kPa	ADC keskmine	ADC müra (std)
2ml	25.3	290	3.1
5ml	101.3	671	4.2
8ml	405.2	2204	8.5
kPa per ADC ühik saad linear regression abil:
kPa_values = [25.3, 101.3, 405.2]
adc_values = [290, 671, 2204]
slope_kPa, intercept_kPa, r_value_kPa, _, _ = linregress(adc_values, kPa_values)

slope_kPa annab sulle: mitu kPa on üks ADC ühik.
Näiteks kui slope_kPa = 0.19, siis üks ADC ühik = 0.19 kPa.
Müra kPa-des
Kui müra on 3.1 ADC ühikut madala rõhu juures ja 0.19 kPa per ADC ühik, siis:
●	Müra = 3.1 × 0.19 = 0.59 kPa
Kui müra on 8.5 ADC ühikut kõrge rõhu juures:
●	Müra = 8.5 × 0.19 = 1.62 kPa
Oluline: müra võib kasvada suurema rõhuga. Põhjused:
●	ESP32 ADC on mittelineaarne - vahemiku otstes on eraldusvõime halvem kui keskel
●	Suurema signaali juures võivad toitepinge kõikumised mõjutada rohkem
●	Op-amp võib sattuda saturatsiooni lähedale ja käituda ebastabiilselt
Kui näed et müra kasvab ühtlaselt rõhuga, siis peamine süüdlane on tõenäoliselt ESP32 ADC mittelineaarsus, mitte andur ise.
Kogu Pilt Korraga
Sul on nüüd kaks numbrit:
1. Teoreetiline eraldusvõime:
●	12-bit ADC annab 4096 võimalikku väärtust
●	Anduri vahemik on 0-700 kPa
●	Teoreetiline: 700/4096 = 0.17 kPa per ADC ühik
See eeldab et:
●	Andur kasutab täielikult kogu 0-700 kPa vahemikku
●	ADC kasutab täielikult kogu 0-3.3V vahemikku
●	Signaal skaleeritakse täpselt õigesse vahemikku
●	Kõik on ideaalne
2. Praktiline eraldusvõime:
Sõltub sellest kas kasutad op-amp'i või pingejagurit.
Näide A: Op-amp (LM358N)
Linear regression annab näiteks 0.19 kPa per ADC ühik (veidi halvem kui teooria, sest op-amp ei kasuta kogu väljundi vahemikku - ei ole rail to rail).
Müra:
●	Madal rõhk (2ml): 3.1 ADC ühikut → 3.1 × 0.19 = 0.59 kPa
●	Keskmine rõhk (5ml): 4.2 ADC ühikut → 4.2 × 0.19 = 0.80 kPa
●	Kõrge rõhk (8ml): 8.5 ADC ühikut → 8.5 × 0.19 = 1.62 kPa
Vahe tuleb sellest et:
●	Op-amp ei kasuta kogu väljundi vahemikku (jääb ~0.02V kuni 3.5V vahele 5V toitel)
●	Op-amp lisab oma müra
●	Kaks inverteerimisetappi võimendavad müra
●	ESP32 ADC mittelineaarsus sööb eraldusvõime
Näide B: Pingejaguriga
Pingejagur skaleeerib anduri 0.2V-4.7V signaali ADC 0-3.3V vahemikku. Näiteks 2:3 jagur (10kΩ + 20kΩ) annab:
●	0.2V → 0.13V
●	4.7V → 3.13V
Linear regression annab näiteks 0.21 kPa per ADC ühik (halvem kui op-amp, sest kasutad vähem ADC vahemikku - ainult ~0.13V kuni 3.13V asemel täiest 0-3.3V).
Müra:
●	Madal rõhk (2ml): 2.8 ADC ühikut → 2.8 × 0.21 = 0.59 kPa
●	Keskmine rõhk (5ml): 3.5 ADC ühikut → 3.5 × 0.21 = 0.74 kPa
●	Kõrge rõhk (8ml): 4.2 ADC ühikut → 4.2 × 0.21 = 0.88 kPa
Vahe tuleb sellest et:
●	Pingejagur vähendab signaali JA müra võrdselt (SNR jääb samaks, aga kasutad vähem ADC vahemikku)
●	Lihtsam vooluahel, vähem komponente mis saavad lisada müra
●	Aga: ei saa optimaalselt kasutada kogu ADC vahemikku
●	Müra jääb väiksemaks kui op-amp versioonil
Oluline järeldus:
Pingejagur annab väiksemat müra (stabiilsem), aga halvema kPa/ADC suhte, sest ei kasuta kogu ADC vahemikku optimaalselt.
Op-amp annab parema kPa/ADC suhte (efektiivsem ADC kasutus), aga suurema müra, eriti rõhu  kõrgemas otsas.

Efektiivne Bittide Arv (ENOB)
Kui tahad olla professionaalne, arvuta ENOB. See on number mis ütleb kui palju su ADC tegelikult suudab.
Signal-to-noise ratio (SNR):
SNR = signaali amplituud / müra amplituud
Näiteks kõrge rõhu juures:
●	Signaal: 2204 ADC ühikut
●	Müra: 8.5 ADC ühikut
●	SNR = 2204 / 8.5 = 259
Detsibellides: SNR_dB = 20 × log₁₀(259) = 48.3 dB
ENOB valem: ENOB = (SNR_dB - 1.76) / 6.02
ENOB = (48.3 - 1.76) / 6.02 = 7.7 bits
Mis see tähendab:
Kuigi ADC on 12-bitine (4096 tasandit), müra tõttu saad praktiliselt ainult 7.7 biti jagu informatsiooni (ligikaudu 200 eristatavat tasandit).
Teooria ütleb 12 bitti. Praktika annab 7.7 bitti. Kaotasid 4.3 bitti müra tõttu.
Kuidas ENOB-i Kasutada Praktikas
ENOB ei ole lihtsalt ilus number - see ütleb sulle konkreetselt mida sa tõesti suudad.
Näide 1: Kas tasub kasutada 16-bitist ADC-d?
Sul on 12-bitine ESP32 ADC (tegelik ENOB ~7.7 bitti). Mõtled osta 16-bitist välise ADC (ADS1115).
Arvutus:
●	16-bitine ADC teoreetiliselt: 65536 tasandit
●	Aga kui sinu anduri müra on ikka 8.5 ADC ühikut, siis ENOB jääb samaks: ~7.7 bitti
●	Tulemus: Mõttetu kulutus. Müra on sinu piirang, mitte ADC bittide arv.
Lahendus: Enne ADC uuendamist:
1.	Paranda müra taset (stabiilsem toitepinge, filtrid, varjestatud kaablid)
2.	Kasuta madala müraga op-amp'i (nt OPA2134)
3.	Lisa analoog low-pass RC filter (flastad)
Kui oled müra alla saanud (näiteks 2 ADC ühikut), SIIS tasub 16-bitist ADC-d kaaluda.
Näide 2: Kui kiiresti saad mõõta?
ENOB ütleb sulle ka kui palju keskmistamist vajad.
Üks mõõtmine: ENOB = 7.7 bitti (200 tasandit)
Kui keskmistada N mõõtmist, siis müra väheneb √N korda:
●	4 mõõtmise keskmine: müra väheneb 2x → ENOB = 8.7 bitti
●	16 mõõtmise keskmine: müra väheneb 4x → ENOB = 9.7 bitti
●	64 mõõtmise keskmine: müra väheneb 8x → ENOB = 10.7 bitti
Praktiline kasutus:
Kui tahad ENOB 10 bitti (1024 tasandit), pead keskmistama ~50 mõõtmist.
Kui mõõdad 5 Hz sagedusel, siis 50 mõõtmist = 10 sekundit. See on sinu süsteemi reaktsiooniaeg.
Kas saad endale lubada 10 sekundit? Kui ei, siis pead müra mujalt vähendama, mitte keskmistamise kaudu.
Näide 3: Disaini otsuste tegemine
Sul on kaks varianti:
Variant A: Pingejagur + 12-bit ADC
●	Lihtsam, odavam
●	ENOB = 8.2 bitti (müra väiksem, aga kasutad vähem ADC vahemikku)
Variant B: Op-amp + 12-bit ADC
●	Keerulisem, kallim
●	ENOB = 7.7 bitti (rohkem müra, aga kasutad rohkem ADC vahemikku)
Otsus:
Kui su rakendus vajab ainult 8 biti täpsust (256 tasandit), siis Variant A piisab. Lihtne ja odav.
Kui vajad 10 biti täpsust (1024 tasandit), siis Variant B + 50x keskmistamine või välismine 16-bit ADC + müra vähendamine.
ENOB teeb need otsused numbrilisteks, mitte intuitsioonipõhisteks.
Näide 4: spetsifikatsiooni kirjutamine
Kui kirjutad oma doseerimissüsteemi spetsifikatsiooni, ära ütle:
"12-bitine resolutsioon" -> "Efektiivne eraldusvõime: 8 bitti (ENOB), ~0.6 kPa madala rõhu juures"
See on aus. See ütleb kliendile mis ta TEGELIKULT saab.
Kalibratsioon
Kui kasutad op-amp vooluahelat (LM358N), siis pead kalibreerima teistmoodi kui lihtsa pingejaguriga. Samas see protsess sobib ka pingejaguriga kasutamiseks.
Probleem: Kõiksugune elektroonika ebatäpsus - anduris kuni digitaalse signaalitöötluseni - on veidi ebatäpne. Oleks vaja süsteem võimalikult sõltumatult kalibreerida.
Kasuta ainult oma kolme punkti:
●	2ml, 5ml, 8ml kompressioon
●	ADC väärtused pärast anduri ja elektroonilist ahelat
Tee linear regression. See annab sulle uue kordaja ja nihke. Need arvutad sõltumatult spetsifikatsioonist.
Vahemiku kontrolli:
Siis võta 9ml kompressioon (rõhk mis peaks satureerima anduri - 912 kPa, mis on üle 700 kPa piiri). Loe ADC väärtus. See peaks olema anduri absoluutne maksimum - andur on täielikult satureerunud.
Seejärel korda sama katset 1ml rõhuga - see on süsteemi miinimum väärtus (~11 kPa).
Kui maksimum pinge jääb liiga madalaks:
Probleem: ADC maksimum 9ml juures on näiteks ainult 2000 ühikut (asemel ~3800). Andur ei jõua saturatsioonini.
Põhjused ja lahendused:
1. Op-amp satureerub enne kui andur
●	Põhjus: Toitepinge liiga madal või op-amp ei suuda väljundis kõrgele ronida
●	See on su PEAMINE probleem - op-amp lõikab signaali enne kui andur satureerub
●	Kontroll: Mõõda ossilloskoobiga op-amp väljundpinge 9ml juures. Kui see on ~3.5V (5V toitel) või ~1.8V (3.3V toitel), siis op-amp satureerub enne andurit
●	Lahendus:
○	Tõsta toitepinget (kasuta 5V, mitte 3.3V)
○	Vähenda teise astme võimendust
○	Kasuta rail-to-rail op-amp'i (nt MCP6002)
2. Surnud maht torudes on liiga suur
●	Põhjus: Pikad/paksud torud, andur ise, kiirühendused - kõik see lisab mahtu
●	10ml süstal + 2ml surnud mahtu = kokku 12ml algmahtu
●	9ml kompressioon: 12ml → 3ml = ainult ~405 kPa, mitte 912 kPa
●	Lahendus: Kasuta lühemaid/õhemaid torusid, eemalda üleliigsed kiirühendused
3. Õhuleke
●	Põhjus: Ühendused pole tihedast, termorüüs on lõdvaks läinud
●	Kontroll: Kas rõhk langeb kiiresti pärast surumist?
●	Lahendus: Kontrolli kõiki ühendusi, vaheta termorüüsi
Oluline: 9ml juures PEAB andur satureeruma. Kui ei satureerugi, siis midagi on valesti ja sa ei kasuta anduri täit vahemikku.

Kui maksimum pinge jääb liiga madalaks:
Probleem: ADC maksimum on näiteks ainult 2000 ühikut (asemel ~3800). Kaotad ära ülemise poole ADC vahemikust.
Põhjused ja lahendused:
1. Op-amp satureerub enne kui andur
●	Põhjus: Toitepinge liiga madal või op-amp ei suuda väljundis küllalt kõrgele ronida
●	Kontroll: Mõõda ossilloskoobiga op-amp väljundpinge. Kui see on ~3.5V (5V toitel) või ~1.8V (3.3V toitel), siis satureerub
●	Lahendus:
○	Tõsta toitepinget (kasuta 3.3V asemel 5V)
○	Tõsta võimendust (muuda op-amp sisendtakististe suhet)
○	Kasuta rail-to-rail op-amp'i (nt MCP6002)
2. Pingejagur vääralt arvutatud
●	Põhjus: Anduri 4.7V skaleeriti liiga vähe alla
●	Kontroll: Mõõda pingejagurit ossilloskoobiga - kas 4.7V sisend annab ~3.2V väljundi?
●	Lahendus: Muuda takistite suhet (näiteks 10kΩ:20kΩ asemel 10kΩ:15kΩ)
3. Andur ei jõua 700 kPa-ni
●	Põhjus: Liiga palju surnud mahtu torudes või õhulekib
●	Kontroll: Kas 9ml kompressioon annab tegelikult anduri maksimaalse väljundi? Mõõda otse andurilt.
●	Lahendus: Kasuta võmalikult lühikest toru, või arvuta toru maht ka süstla skaalale juurde
Kui miinimum pinge jääb liiga kõrgeks:
Probleem: ADC miinimum on näiteks 500 ühikut (asemel ~50). Kaotad ära alumise osa ADC vahemikust.
Põhjused ja lahendused:
1. Nihkepinge liiga kõrge
●	Põhjus: Op-amp esimese astme nihkepinge on liiga kõrge
●	Kontroll: Mõõda pingejagarit mis annab nihkepinge
●	Lahendus: Vähenda nihkepinget 
2. Andur ei lähe nulli lähedale
●	Põhjus: Atmosfäärirõhk on juba baastasemel (gauge pressure sensor)
●	See on normaalne: MPX5700AP mõõdab ülerõhku, mitte absoluutset rõhku
●	Lahendus: Pole vaja parandada - see on disain
*selle vabanduse taha peidavad insenereid väga plju hirmsaid asju
Ideaalne vahemik:
Sinu ADC väärtused peaksid olema:
●	Miinimum (1ml): ~100-300 ADC ühikut
●	Maksimum (9ml): ~3500-3900 ADC ühikut
See kasutab ~85-95% ADC vahemikust. Jätab veidi puhverruumi müra ja kõikumiste jaoks.
Kui su vahemik on sellest väiksem, tasub vooluahelat optimeerida. Kui on selles vahemikus, on piisavalt hea.

Python 
Kogu analüüs ühes jupis:
import numpy as np
from scipy.stats import linregress
import matplotlib.pyplot as plt

# Toorandmed (asenda oma numbritega)
compression_ml = np.array([2, 5, 8])
theoretical_kPa = np.array([25.3, 101.3, 405.2])

# 10 mõõtmist iga taseme kohta
adc_2ml = np.array([285, 292, 288, 295, 290, 287, 293, 289, 291, 294])
adc_5ml = np.array([665, 671, 668, 673, 670, 669, 672, 667, 674, 671])
adc_8ml = np.array([2198, 2204, 2201, 2207, 2200, 2203, 2206, 2199, 2205, 2202])

# Arvuta keskmised ja müra
mean_adc = np.array([np.mean(adc_2ml), np.mean(adc_5ml), np.mean(adc_8ml)])
std_adc = np.array([np.std(adc_2ml), np.std(adc_5ml), np.std(adc_8ml)])

print("=== ADC VÄÄRTUSED ===")
for i, comp in enumerate(compression_ml):
   print(f"{comp}ml kompressioon: keskmine={mean_adc[i]:.1f}, std={std_adc[i]:.2f}")

# Linear regression: ml -> ADC
slope_ml_to_adc, intercept_ml, r_ml, _, _ = linregress(compression_ml, mean_adc)
print(f"\n=== KOMPRESSIOON -> ADC ===")
print(f"Kordaja: {slope_ml_to_adc:.1f} ADC ühikut per ml")
print(f"R²: {r_ml**2:.4f}")

# Linear regression: ADC -> kPa
slope_adc_to_kPa, intercept_kPa, r_kPa, _, _ = linregress(mean_adc, theoretical_kPa)
print(f"\n=== ADC -> kPa ===")
print(f"Kordaja: {slope_adc_to_kPa:.4f} kPa per ADC ühik")
print(f"R²: {r_kPa**2:.4f}")
print(f"Teoreetiline (spec): {700/4096:.4f} kPa per ADC ühik")

# Resolutsioon
print(f"\n=== ERALDUSVÕIME ===")
for i, comp in enumerate(compression_ml):
   res_ml = std_adc[i] / slope_ml_to_adc
   res_kPa = std_adc[i] * slope_adc_to_kPa
   print(f"{comp}ml: müra={std_adc[i]:.2f} ADC → {res_ml:.4f} ml → {res_kPa:.3f} kPa")

# ENOB (võta parim juhtum - madalaim müra)
snr = mean_adc[0] / std_adc[0]
snr_db = 20 * np.log10(snr)
enob = (snr_db - 1.76) / 6.02
print(f"\n=== ENOB (madala rõhu juures) ===")
print(f"SNR: {snr_db:.1f} dB")
print(f"ENOB: {enob:.1f} bits (teoreetiline 12 bits)")

# Graafik
plt.figure(figsize=(10, 6))
plt.errorbar(compression_ml, mean_adc, yerr=std_adc, fmt='o', capsize=5, label='Mõõdetud')
x_fit = np.linspace(0, 10, 100)
y_fit = slope_ml_to_adc * x_fit + intercept_ml
plt.plot(x_fit, y_fit, 'r--', label=f'Linear fit (R²={r_ml**2:.4f})')
plt.xlabel('Kompressioon (ml)')
plt.ylabel('ADC väärtus')
plt.legend()
plt.grid(True)
plt.title('Kompressioon vs ADC väärtus')
plt.show()

Tulemused 
Pärast seda analüüsi peaksid esitama:
1. Üks koodiväärtus on mitu kPa:
●	Näide: 0.19 kPa per ADC ühik (praktiline)
●	Võrdle: 0.17 kPa per ADC ühik (teoreetiline spetsifikatsioonist)
2. Mitu koodiväärtust on müra tase:
●	Madal rõhk (2ml): 3.1 ADC ühikut
●	Keskmine rõhk (5ml): 4.2 ADC ühikut
●	Kõrge rõhk (8ml): 8.5 ADC ühikut
3. Praktiline eraldusvõime:
●	Madal rõhk: 0.59 kPa
●	Keskmine rõhk: 0.80 kPa
●	Kõrge rõhk: 1.62 kPa
4. ENOB:
●	7-8 bitti (sõltuvalt rõhust)
5. Lineaarsuse hinnang:
●	R² > 0.99: süsteem on lineaarne
●	R² < 0.99: süsteem pole lineaarne, vaja uurida põhjust

