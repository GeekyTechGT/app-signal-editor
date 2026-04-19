COMPLETED UPDATES:
- plot toolbar moved into a dedicated themed control card instead of overlapping the plot
- flexible decimal parsing now accepts both `.` and `,` without corrupting small values
- plot zoom now keeps samples, handles, and fill regions inside the visible plot bounds
- zoom in / zoom out are available from both toolbar buttons and mouse wheel
- the plot toolbar now includes `Pan mode`, `Rect mode`, and `Fit view`
- interpolation changes preserve the active time zoom instead of forcing fit view
- tooltip and hover hints were added across the main workspace and editing panels
- Italian and English translations now load at startup and switch live from the settings panel

Vorrei che implementassi un app in c++23, seguendo principi solid, kiss, dry, per replicare il tool signal editor di matlab (link: https://www.mathworks.com/help/simulink/slref/signaleditortool.html). L'obiettivo iniziale è poter caricare (tramite filesystem o drag and drop ) un file csv, contenente la prima colonna il time, e le colonne successive le y(t), dove ogni colonna rappresenta un segnale.
L'utente deve poter visualizzare in una list tutti i segnali caricati, e per ciascuno poter visualizzare la forma d'onda, e tramite mouse con drag e drop poter modificare la morfologia della forma donda, aggiunge waypoint, tirare la curva e cambiare morfologia, e poter salvare la nuova forma d'onda; l'utente deve poter esportare le modifiche fatte in formato csv. Inoltre deve poter aggiungere un segnale from scratch, riumovere segnale esistente, o modificare un segnale estratto dal file. Fai tutto ispirandoti a Signal editor di matlab. Utilizza la struttura definita in questo progetto, privilegiando architettura esagonale, quindi tutti i file .hpp e .cpp dentro la folder /src seguendo la struttura esagonale. Dentro app devi inserire il main, per ora crea solo app gui. Utilizza Qt per la grafica. Crea gui accattivante e moderna.

-----------------------------------------
Vorrei che migliorassi l'app applicando le seguenti modifiche:
1. rimuovi tutti gli scripts .bat e alliena il progetto alla struttura di: D:\GeekyTechRepos\lib-qt-utils\scripts , quindi rimuovi il file project_manager.bat , aggiungi il file requirements.txt per creare il virtual env, aggiorna il README per dettagliare la procedura di setup (prende esempio da lib-qt-utils): assicurati che tutto funziona correttamente
2. crea la folder external dove inserisci il repo D:\GeekyTechRepos\res-qt-themes
3. utilizza i files dark.qss e light.qss definiti in res-qt-theme
4. utilizza le librerie lib-qt-utils e lib-qt-custom-widgets (per la settings page) prendendo i bin da D:\GeekyTechRepos\_shared_lib -> devi creare nella root del progetto un file di config json (esempio: D:\GeekyTechRepos\lib-qt-utils\project.json) dove inserisci in dependencies tutte le dipendeze, nome libria e versione, non il preset perche devi concatenare quello che stai utilizzando: quindi se usi windows-mingw64-release e in shared_folder hai windows-mingw64-debug, devi bloccare tutto e mostrare errore verboso.
5. rimuovi lo stylesheet hardcored nel codice perche devi utilizzare i file qss definiti in res-qt-themes
6. crea un settings button nella status bar in basso a sinsitr (nel corner) che se cliaccata apre la settings page definita nella lib lib-qt-custom-windgets: sincronizza la main window ai settings definiti
7. gestisci l'internazionalizzazione dell'app come un professionista (per ora solo italiano e inglese): crea i file di traduzioni, quindi non hardcodare nel codice i testi: gestisci basandoti sul framework qt su come gestisce questa cosa
8. rimuovi tutti i files non piu utilizzati
9. aggiorna il README in modo professionale, con badges e tutti i dettagli dell'app .

aggiungi la folder resources, dove metti la folder img, translations .
Dentro img devi inserire il logo dell'app.

migliora il colore dello splash screen, perche non si legge il testo su background nero: fammelo piu accattivante


fai le modifiche necessarie per riflettere tutti i cambiamenti di settings_panel: tutti i segnali che emette li devi gestire correttamente. Prendi esempio da: D:\GeekyTechRepos\lib-qt-custom-widgets\examples\03_settings_panel\main.cpp e leggi la classe D:\GeekyTechRepos\lib-qt-custom-widgets\src\widgets\settings_panel\settings_panel_dialog.cpp


-----------------------
applica le seguenti modifiche, in modo autonomo, senza farmi domande, e senza rompere le logiche già implementate, rispettando principi SOLID, DRY, KISS e architettura esagonale:
- la tool bar si sovrappone al plot, e ha un background nero: fai tutto in modo piu organizzato e strutturato e accatttivnate e moderno.
- nelle line edit gestisci sia il . che la , : se adesso inserisco 0.0005 viene convertito a 5. Fai le modifiche richieste.
- quando faccio zoom in i samples escono dalla finestra del plot (nei bordi destro e sinistro). Ottimizza e migliora.
- quando l'utente crea un nuovo segnale viene creato 'untitled workspace' . Aggiungi la possibilità di rinominare i files importati e il progetto creato from scratch.
- devi abilitate zoom in e zoom out anche con la wheel del mouse.
- le label (esempio nome del segnale) nel plot devono poter essere spostate a piacemento dall'utente, e messe in punti del plot dove vuole: deve poterle trascinare con il mouse: quando rilasciate devono ancorarsi nel punto definito.

per ogni elemento della UI vorrei aggiungessi tooltip e hint che si mostrano quando l'utente fa hover con il mouse sopra l'oggetto. Prevedi le traduzioni.
