WARNING: This translation is out of date.

	Πώς να εγκαταστήσετε την Debian Sarge με τον καινούριο debian-install
	---------------------------------------------------------------------
	
	
	
	Το κείμενο αυτό περιγράφει τον τρόπο εγκατάστασης με τον καινούριο 
	debian-installer, που θα κυκλοφορήσει μαζί με την επόμενη έκδοση της Debian
	με το κωδικό όνομα: sarge.
	
	Για την πιο πρόσφατη εκδοση αυτού του κειμένου και περισσότερες πληροφορίες
	για τον debian-installer επισκεφθείτε τη σελίδα:
	http://www.debian.org/devel/debian-installer

	Τελευταία αλλαγή στο κείμενο αυτό: 22 Δεκεμβρίου, 2003
	
	
	1. Προκαταρκτικά
	
	Εικόνες του debian-installer είναι προς το παρόν διαθέσιμες μόνο για τις
	αρχιτεκτονικές i386, ppc, alpha και ia64. Διάθεση και για άλλες
	αρχιτεκτονικές είναι φυσικά ευπρόσδεκτη! Δείτε την παράγραφο 6 αν
	θέλετε να βοηθήσετε στην ανάπτυξη.
	
	
	Ο debian-installer είναι ακόμα σε στάδιο beta. Αν συναντήσετε σφάλματα κατά την
	εγκατάστασή σας κοιτάξτε την παράγραφο 5 σχετικά με το πώς να τα
	αναφέρετε. Αν έχετε ερωτήματα που δεν μπορούν να απαντηθούν από το
	κείμενο αυτό, παρακαλώ απευθύνετε τα στη λίστα debian-boot 
	(debian-boot@lists.debian.org) ή ρωτήστε στο κανάλι irc 
	(#debian-boot στο freenode network).
	
	
	Πρόσφατα ο debian-installer έχει αλλάξει ώστε να ρωτά μόνο τις σημαντικές
	ερωτήσεις και να κάνει τις υπόλοιπες ρυθμίσεις αυτόματα. Αυτό σημαίνει
	ότι δεν θα μπορείτε πλέον να δειτε το κυρίως μενού παρά μόνο αν κάτι
	πάει λάθος. Αν θέλετε να δείτε το παλιό μενού ρυθμίσεων με
	περισσότερες ερωτήσεις πληκτρολογήστε 'expert' κατά
	την προτροπή εκκίνησης. Αν επιλέξτε κάτι τέτοιο τότε κοιτάξτε την παράγραφο 3.1
	μάλλον παρά την 3 για οδηγίες.
	
	
	2. Αποκτώντας τις εικόνες του debian-installer
	
	Η ομάδα ανάπτυξης του debian-installer διαθέτει εκδόσεις εικόνων CD
	για τον debian-installer στη διεύθυνση:
	http://people.debian.org/~manty/testing/
	
	Τα άλλα είδη εικόνων, συμπεριλαμβανομένων των εικόνων για δισκέττες και
	τα initrd είναι διαθέσιμα στο κύριο αρχείο της Debian, στους
	αντίστοιχους καταλόγους main/installer-<arch>. Για παράδειγμα: 
	ftp://ftp.debian.org/debian/dists/unstable/main/installer-i386/current/images/
	
	Οι ημερήσιες εκδόσεις όλων των non-ISO εικόνων του debian-installer,
	συμπεριλαμβανομένων των εικόνων για δισκέττες και initrd είναι
	διαθέσιμα στις διευθύνσεις:
	για i386:    http://people.debian.org/~joeyh/d-i/images/daily
	για  powerpc: http://people.debian.org/~tsauter/d-i/images-powerpc/daily/
	(μόνο για cdrom και netboot).
	
	Οι υποενότητες που ακολουθούν δίνουν τις λεπτομέρειες για το ποιές
	ακριβώς εικόνες θα χρειαστείτε για το κάθε αντίστοιχο μέσο εγκατάστασης.
	
	
	2.1 CDROM
	
	Υπάρχουν δυο διαφορετικές εικόνες εγκατάστασης από το δίκτυο (netinst
	images) στην τοποθεσία που προαναφέραμε και που μπορούν να χρησιμοποιηθούν για την
	εγκατάσταση του sarge με τον debian-installer. Η χρησιμότητα αυτών των εικόνων είναι να
	γίνει η εκκίνηση από το CDROM και στη συνέχεια τα επιπρόσθετα πακέτα να
	εγκατασταθούν από το δίκτυο, γιαυτό και η ονομασία netinst. Η διαφορά
	ανάμεσα στις δυο εικόνες είναι ότι στο πλήρες netinst cd περιέχονται
	όλα τα πακέτα του βασικού συστήματος, ενώ με το business card cd αυτά τα
	πακέτα θα πρέπει να κατεβούν από το διαδίκτυο.
	
	Κατεβάστε όποιο τύπο προτιμάτε και "κάψτε" το σε ένα CD.
	
	
	2.1.1 Οδηγοί SCSI CD
	
	Αν έχετε SCSI οδηγό CD τότε θα πρέπει επίσης να κατεβάσετε μια
	δισκέττα με οδηγούς ώστε να μπορέσει ο εγκαταστάτης να "δει" τη συσκευή για
	το CD-ROM. Δείτε την ενότητα 2.2 για πληροφορίες σχετικά με δισκέττες,
	κατεβάστε την εικόνα δισκέττας με τους οδηγούς scsi και γράψτε το σε
	μια δισκέττα. Μετά την αρχική αποτυχία του εγκαταστάτη να "δει"  το
	SCSI CD έχετε τη δυνατότητα φορτώνοντας τους οδηγούς από τη δισκέττα
	να δει ο εγκαταστάτης στη συνέχεια το CD ROM κανονικά. 
	
	
	2.2 Οι "άβολες" δισκέττες
	
	Αν δεν μπορείτε να ξεκινήσετε απο ένα CD μπορείτε να κατεβάσετε
	εικόνες δισκεττών για να εγκαταστήσετε τη Debian. Θα χρειαστείτε το
	bootfloppy-image.img, το floppy-image.img και πιθανά μια από τις
	δισκέττες με οδηγούς.
	
	--οδηγοί δικτύου
	Δε θα χρειαστείτε αυτή τη δισκέττα αν κάνετε εγκατάσταση μέσω
	δικτύου και χρησιμοποιείτε κάποιες από τις πιο κοινές κάρτες
	ethernet. Αν όμως έχετε κάποια λιγότερη γνωστή κάρτα δικτύου, ή μια
	κάρτα pcmcia, τότε θα τη χρειαστείτε.
	
	--οδηγοί cd
	Αν έχετε ένα CDROM αλλά δεν μπορείτε να εκκινήσετε από αυτό τότε
	μπορείτε να εκκινήσετε από τις δισκέττες και να χρησιμοποιήσετε αυτή
	τη δισκέττα οδηγών για να ολοκληρώσετε την εγκατάσταση από το CDROM.
	
	Οι δισκέττες είναι από τα λιγότερο αξιόπιστα μέσα επομένως καλό είναι να είστε 
	προετοιμασμένοι για αρκετές ελλαττωματικές από αυτές. Κάθε .img αρχείο που
	κατεβάσατε χωράει σε μια δισκέττα. Μπορείτε να χρησιμοποιήσετε την εντολή dd για να το
	γράψετε στο /dev/fd0 η κάποιο άλλο μέσο. Είναι καλό επίσης να χρησιμοποιήσετε το εργαλείο
	cmp για να συγκρίνετε το αρχείο .img με το αποτέλεσμα της εγγραφής στην (όχι πολύ
	αξιόπιστη) δισκέττα. Αν η σύγκριση αποτύχει τότε πετάξτε τη δισκέττα και
	ξαναπροσπαθήστε. Μιας και θα έχετε περισσότερες από μια
	δισκέττες είναι καλή ιδέα να τις ξεχωρίσετε με κάποια ετικέττα.
	
	Η δισκέττα εκκίνησης (boot floppy) είναι αυτή με το αρχείο bootfloppy-image.
	Αυτή η δισκέττα, μετά την εκκίνηση, θα σας παρακινήσει να βάλετε και μια δεύτερη
	δισκέττα-- χρησιμοποιήστε αυτήν όπου έχετε γράψει το floppy-image. 
	
	
	2.3 USB stick μνήμης
	
	Η εγκατάσταση είναι επίσης δυνατή και από μια αφαιρέσιμη USB συσκευή
	αποθήκευσης. Για παράδειγμα ένα "κλειδί" USB μπορεί να γίνει ένα πολύ
	βολικό μέσο εγκατάστασης της Debian που μπορεί κάποιος εύκολα να
	το πάρει μαζί του οπουδήποτε. 
	
	Ο ευκολότερος τρόπος για την προετοιμασία του USB stick είναι να κατεβάσετε το
	αρχείο hd-media-image.img.gz και να χρησιμοποιήσετε το gunzip για να αποσυμπιέσετε 
	την εικόνα των 128 MB από το αρχείο αυτό. Γράψτε την εικόνα κατευθείαν στο stick μνήμης, το οποίο
	θα πρέπει βέβαια να έχει μέγεθος τουλάχιστον 128 ΜΒ. Στη συνέχεια 
	προσαρτήστε το stick μνήμης, που τώρα θα διαθέτει έτσι ένα σύστημα αρχείων FAT.
	Στη συνέχεια κατεβάστε μια εικόνα του Debian netinst CD και αντιγράψτε το
	στο stick μνήμης. Οποιοδήποτε όνομα αρχείου είναι κατάλληλο αρκεί να
	έχει την κατάληξη ".iso".
	
	Εναλλακτικά, αν χρησιμοποιείτε Linux και είσαστε εξοικειωμένοι με την
	προσάρτηση loopback συσκευών, είναι γρηγορότερο να προσαρτήσετε την εικόνα του 
	CD σαν loopback συσκευή, να αντιγράψετε το αρχείο iso σε αυτήν και μόνο τότε
	να γράψετε την πλήρη εικόνα στο stick μνήμης.
	
	Υπάρχουν και άλλοι πιο ευέλικτοι τρόποι για ρυθμίσετε ένα stick μνήμης ώστε να
	χρησιμοποιήσετε τον Debian installer. Είναι ακόμα δυνατόν να καταφέρετε να
	δουλεψέτε και με μικρότερα stick μνήμης. Αυτή η σελίδα στο διαδίκτυο περιέχει
	πιο ολοκληρωμένες οδηγίες για τη χρήση του
	debian-installer και ενός εκκινήσιμου USB stick: http://d-i.pascal.at/
	
	
	2.3.1 Εκκίνηση κατευθείαν από τη συσκευή USB
	
	Mερικά BIOS μπορούν να εκκινήσουν κατευθείαν από μια συσκευή αποθήκευσης USB
	ενώ κάποια άλλα όχι. Ίσως χρειαστεί να ρυθμίσετε το BIOS ώστε να εκκινεί από
	μια "αφαιρέσιμη συσκευή" ή ακόμα και ένα "USB-ZIP" για να κάνετε το σύστημά σας
	να εκκινήσει από τη συσκευή USB. Η ιστοσελίδα που αναφέρθηκε παραπάνω έχει
	περισσότερες πληροφορίες καθώς και κάποιες χρήσιμες υποδείξεις σχετικά με την
	εκκίνηση ενός συστήματος. 
	
	2.3.2 Χρησιμοποιώντας μια USB συσκευή και μια δισκέττα εκκίνησης
	
	Ο debian-installer μπορεί να εκκινηθεί και από μια μονή δισκέττα που μπορεί να
	έχει πρόσβαση στο USB stick μνήμης. Για να γίνει αυτό πρέπει να τοποθετήσετε το
	αρχείο bootfloppy-image.img σε μια δισκέττα (δες ενότητα 2.2)
	
	Τώρα εκκινήστε από τη δισκέττα που έχει το bootfloppy-image. Θα είναι σε θέση
	να ανιχνεύσει τη συσκευή USB και να προχωρήσει στην εκκίνηση από αυτήν.
	
	
	2.4 Εκκίνηση από το Δίκτυο
	
	Είναι επίσης δυνατόν να εκκινήσετε τον debian-installer μόνο από
	το δίκτυο. Οι διάφορες μέθοδοι για εκκίνηση από το δίκτυο (netboot) εξαρτώνται
	από την αρχιτεκτονική του υπολογιστή σας και τις ρυθμίσεις του netboot. Δεν
	αναλύονται εδω. Ο Joe Nahmias έστειλε στη λίστα debian-testing οδηγίες σχετικά
	με μια PXE δικτυακή εκκίνηση. Θα τη βρείτε στο:
	http://lists.debian.org/debian-testing/2003/debian-testing-200311/msg00098.html
	
	Το αρχείο "netboot-initrd.gz" είναι απαραίτητο για μια δικτυακή εκκίνηση του
	debian-installer.
	Περιέχει μόνο τα modules εκείνα του debian-installer που είναι απαραίτητα για
	τη ρύθμιση και τη λειτουργία ενός δικτύου.Ο,τιδήποτε άλλο (συνιστώσες του d-i
	και βασικά πακέτα) θα ανακτηθούν από το δίκτυο.
	Αν δεν έχετε ρυθμίσεις για δικτυακή εκκίνηση μπορείτε επίσης να δημιουργήσετε
	ένα εκκινήσιμο CD με αυτό το image αποκτώντας έτσι ένα μικρό CD δικτυακής
	εγκατάστασης (netinst CD).
	
	
	3. Εγκατάσταση
	
	Από δω και στο εξής, θα υποθέσουμε ότι έχετε κατεβάσει και "κάψει" το 
	netinst CD. Tοποθετήστε το στη συσκευή CD και κάντε το σύστημά σας να
	εκκινεί από το CD.
	
	Θα αντικρύσετε στην αρχή μια οθόνη υποδοχής. Πιέστε ENTER για την εκκίνηση.
	Μετά από λίγο θα ερωτηθείτε για την επιλογή της γλώσσας. Αυτό θα επηρεάσει
	την μετάφραση του debian-installer (αν είναι ήδη διαθέσιμη για τη γλώσσα σας)
	καθώς και την επιλογή της διάταξης του πληκτρολογίου. Επιλέξτε τη γλώσσα σας
	και πιέστε ENTER για τη συνέχεια.
	
	Χαλαρώστε λίγο ενόσω ο debian-installer προσπαθεί να ανιχνεύσει το υλικό του υπολογιστή σας 
	και να φορτώσει επιπλέον modules από το CD.
	
	
	Στη συνέχεια ο εγκαταστάτης θα προσπαθήσει να ανιχνεύσει το δικτυακό σας
	υλικό και να ρυθμίσει το δίκτυο με τη χρήση DHCP. Αν δεν είσαστε σε ένα
	δίκτυο ή δεν έχετε DHCP, θα
	δείτε ένα μήνυμα σφάλματος. Δεν χρειάζεστε δίκτυο για να συνεχίσετε την
	εγκατάσταση οπότε
	αυτό παρακάμπτεται εύκολα. Επιλέξτε "Συνέχεια" και παρακολουθήστε το κυρίως μενού
	που εμφανίζεται
	κάθε φορά που κάτι πάει λάθος, ώστε να έχετε μεγαλύτερο έλεγχο της κατάστασης.
	Προχωρήστε στο "Διαμερισμός του δίσκου".
	
	Τώρα είναι η στιγμή να κάνετε το διαμερισμό των σκληρών σας δίσκων. Επιλέξτε το
	δίσκο που θέλετε να διαμερίσετε οπότε ένα πρόγραμμα διαμέρισης κατάλληλο για
	την αρχιτεκτονική σας
	θα ξεκινήσει. Κάντε τη διαμέριση του δίσκου σύμφωνα με τις ανάγκες σας και
	παραιτηθείτε από
	το πρόγραμμα. Επιλέξτε "Τέλος" ("Finish") για να συνεχίσετε.
	
	Στην επόμενη οθόνη θα πρέπει να διαμορφώσετε και να αναρτήσετε τις διαμερίσεις.
	Επιλέξτε τις διαμερίσεις που θέλετε να χρησιμοποιήσετε στο υπό εγκατάσταση σύστημα και
	επιλέξτε ένα σύστημα αρχείων και ένα σημείο ανάρτησης για αυτές.Θυμηθείτε να ορίσετε
	τουλάχιστον μια διαμέριση για χώρο swap και να αναρτήσετε μια διαμέριση στο σημείο "/".
	
	Σημειώστε ότι ο debian-installer δεν θα κάνει καμιά αλλαγή στους δίσκους σας μέχρι να 
	επιλέξετε "Τέλος". Αυτό να το κάνετε μόνο όταν είστε βέβαιοι ότι έχετε βρει
	μια κατάλληλη ρύθμιση για τις διαμερίσεις σας  και απαντήστε την επόμενη ερώτηση με "Ναι".
	
	
	Τώρα ο debian-installer αρχίζει να εγκαθιστά το βασικό σύστημα κάτι που μπορεί να
	πάρει αρκετό χρόνο. Αυτό ακολουθείται από την εγκατάσταση του πυρήνα.
	
	Το τελευταίο βήμα είναι η εγκατάσταση ενός φορτωτή εκκίνησης (boot loader). Θα 
	ερωτηθείτε για το bootblock όπου θα εγκατασταθεί το LILO. Εξ ορισμού αυτό
	είναι το αρχείο εκκίνησης (boot record) του πρώτου σκληρού δίσκου που είναι γενικά μια καλή εκλογή.
	
	Ο debian-installer θα σας πει τώρα ότι η εγκατάσταση τελείωσε. Απομακρύνετε το CD από
	τη συσκευή του και πατήστε ENTER για να επανεκκινήσετε το μηχάνημα σας.
	Βεβαιωθείτε ότι εκκινεί από τον σκληρό δίσκο, σταυρώστε τα δάχτυλα και περιμένετε μέχρι να
	ξεκινήσει η διαμόρφωση του βασικού συστήματος!!! 
	
	Η πορεία μέσω της βασικής διαμόρφωσης (base-config) δεν είναι μέσα στους
	σκοπούς αυτού του κειμένου καθώς αυτή δεν αποτελεί μέρος του debian-installer.
	
	
	3.1 Εγκατάσταση στην κατάσταση expert
	
	Από δω και στο εξής, θα υποθέσουμε ότι έχετε κατεβάσει και "κάψει" το 
	netinst CD. Tοποθετήστε το στη συσκευή CD και κάντε το σύστημά σας να
	εκκινεί από το CD.
	
	Θα αντκρύσετε μια οθόνη υποδοχής. Πληκτρολογήστε "expert" και πατήστε ENTER για
	την εκκίνηση. Μετά από λίγο
	θα δείτε το κυρίως μενού του debian-installer. 
	Μερικές γενικές παρατηρήσεις:
	
	Το κυρίως μενού δεν είναι στατικό. Νέες επιλογές εμφανίζονται καθώς φορτώνονται
	καινούρια modules του εγκαταστάτη.Παρόλα αυτά ο εγκαταστάτης προσπαθεί 
	να αποφασίσει για την επόμενη καλύτερη δυνατή επιλογή και την εμφανίζει
	σαν την αυτόματη επιλογή. Αν αυτή η επιλογή δεν σας βολεύει μπορείτε απλά 
	να κάνετε μια οποιαδήποτε άλλη. Αν επιλέξετε μια δυνατότητα που απαιτεί τη
	ρύθμιση
	μιας άλλης επιλογής που δεν έχετε ακόμα επιλέξει,ο εγκαταστάτης θα προσπαθήσει
	αυτόματα 
	να επιλύσει τις αλληλεξαρτήσεις αυτές. Αυτό μπορεί να χρησιμοποιηθεί για την
	αυτοματοποίηση της διαδικασίας εγκατάστασης, με την επιλογή πάντα του πιο
	πρόσφατου "εμφανούς" βήματος.
	
	Όταν πρωτοεμφανίζεται το κυρίως μενού η αυτόματη επιλογή είναι η
	"Επιλογή Γλώσσας". Πατήστε RETURN και επιλέξτε τη γλώσσα από τη λίστα που
	θα εμφανιστεί. Θα οδηγηθείτε τότε πίσω στο κυρίως μενού και η επόμενη επιλογή
	τώρα θα είναι η "Ανίχνευση πληκτρολογίου και επιλογή διάταξης".
	
	Επιλέξτε αυτό το βήμα και παρατηρήστε ότι ο εγκαταστάτης προσπαθεί να
	θέσει μια εύλογη εξορισμού εκλογή με βάση την επιλογή γλώσσας που έχετε ήδη
	κάνει. Επιλέξτε το πληκτρολόγιο της προτίμησής σας και συνεχίστε.
	
	Το επόμενο βήμα ειναι "Ανίχνευση συσκευών CDROM και ανάρτηση του CD".
	Αυτό το βήμα δεν απαιτεί καμμιά ενέργεια από την πλευρά του χρήστη, όλα
	γίνονται αυτόματα.
	
	Τώρα είμαστε σε θέση να φορτώσουμε τα υπόλοιπα μέρη του εγκαταστάτη. Κάνετε 
	την αντίστοιχη επιλογή "Φόρτωση συνιστωσών εγκαταστάτη". Εφόσον τα modules στα
	οποία θέλουμε να έχουμε πρόσβαση βρίσκονται στο CD, επιλέξτε "ανάκτηση από το CD". H
	ανάκτηση-από-δισκέττα μπορεί να χρησιμοποιηθεί για τη φόρτωση επιπρόσθετων
	modules, πχ. αν έχετε κάποια εξωτική συσκευή. Δείτε την ενότητα 2.1.1 αν έχετε
	ένα SCSI CDROM.
	
	Εμφανίζεται μια μεγάλη λίστα με προαιρετικές συνιστώσες για εγκατάσταση.
	Θέλουμε να εγκαταστήσουμε μόνο τις καθιερωμένες συνιστώσες, οι οποίες
	επιλέγονται αυτόματα, επομένως
	απλά χτυπάμε "Συνέχεια". Περιμένετε και προσέξτε μέχρι να φορτωθούν όλες
	οι συνιστώσες.
	
	Εμφανίζεται πάλι το κυρίως μενού, αλλά τώρα με τα πρόσθετα modules υπάρχουν
	καινούργιες επιλογές. Το επόμενο προεπιλεγμένο βήμα είναι η ρύθμιση του
	δκτύου. Εδώ εγκαταλείπουμε την προκαθορισμένη πορεία μιας και δεν χρειαζόμαστε
	δικτύωση αφού τα βασικά πακέτα βρίσκονται στο CD.
	Eπιλέξτε "Ανίχνευση υλικού". Το βήμα αυτό επίσης δεν απαιτεί
	κάποια αλληλεπίδραση με το χρήστη.
	
	Τώρα είναι η στιγμή για τη διαμερισμό του δίσκου. Υπάρχουν βασικά δυο
	τρόποι για να γίνει αυτό. Ο πρώτος είναι με τη χρήση του cfdisk, που θα
	ξεκινήσει αμέσως με την εκλογή της επιλογής του μενού 
	"Διαμερισμός ενος σκληρού δίσκου". Βεβαιωθείτε να δημιουργήσετε τουλάχιστον
	δυο διαμερίσεις, μια για μνήμη swap και μια για το βασικό (root) σύστημα
	αρχείων.
	
	Η άλλη πιθανότητα είναι η χρήση του autopartitioner. Επιλέξτε 
	"Αυτόματος διαμερισμός σκληρών δίσκων" που προσπαθεί να καθορίσει 
	έναν ασφαλή διαμερισμό. ΠΡΟΕΙΔΟΠΟΙΗΣΗ: Χρησιμοποιήστε αυτή την επιλογή μόνο
	αν έχετε καθόλου ή χωρίς σημασία δεδομένα στους δίσκους σας!!!
	
	Αφού τελειώσετε τον διαμερισμό, επιλέξτε τη "Διαμόρφωση και προσάρτηση
	διαμερίσεων". Μιας και το autopartkit δημιουργεί συστήματα αρχείων και τα
	προσαρτά αυτόματα, μπορείτε να παραλείψετε αυτό το βήμα αν χρησιμοποιήσατε
	το autopartkit για το διαμερισμό του δίσκου σας.
	
	Η διαμόρφωση των διαμερίσεων είναι αρκετά εύκολη. Εμφανίζεται μια
	λίστα με όλες τις διαμερίσεις, τα μεγέθη τους και το σύστημα αρχείων 
	αν κάποιο ανιχνεύθηκε σε αυτές. Η επιλογή μιας διαμέρισης επιτρέπει την
	επιλογή του συστήματος αρχείων που θα δημιουργηθεί σε αυτήν. Αν επιλέξετε 
	ένα σύστημα αρχείων άλλο από swap θα ερωτηθείτε επίσης για το σημείο
	προσάρτησης.
	
	Ρυθμίστε τις διαμερίσεις σύμφωνα με τις ανάγκες σας και θυμηθείτε να
	προσδιορίσετε μια διαμέριση με σημείο προσάρτησης "/". Όταν κάνετε τις
	επιλογές	σας επιλέξτε "Τέλος" και επιβεβαιώστε ότι τα συστήματα αρχείων 
	θα δημιουργηθούν σύμφωνα με τις επιθυμίες σας.
	
	Τώρα είμαστε έτοιμοι για την εγκατάσταση του βασικού συστήματος. Διαλέξτε
	την αντίστοιχη επιλογή ("Εγκατάσταση του βασικού συστήματος") και γείρετε
	πισω! Τα πακέτα ανακτώνται από το CD και εγκαθίστανται στην περιοχή /target.
	
	Το επόμενο βήμα είναι η εγκατάσταση του πυρήνα ("Εγκατάσταση του πυρήνα").
	Θα παρουσιαστεί μια λίστα με όλες τις διαθέσιμες εικόνες πυρήνων που
	βρίσκονται στο CD. Διαλέξτε την καταλληλότερη για το σύστημά σας και περιμένετε
	μέχρι να ολοκληρωθεί η εγκατάσταση.
	
	Έχουμε σχεδόν τελειώσει. Επιλέξτε "Εγκατάσταση του LILO σε σκληρό δίσκο"
	ή "Εγκατάσταση του GRUB σε σκληρό δίσκο" για να καταστήσετε το
	δίσκο σας εκκινήσιμο. Θα ρωτηθείτε για το σημείο εγκατάστασης ενός
	bootblock από το LILO/GRUB. Μια καλή ιδέα είναι ο πρώτος δίσκος στο σύστημά σας
	που είναι άλλωστε και η προκαθορισμένη επιλογή.
	
	Αν το τελευταίο βήμα έχει ολοκληρωθεί με επιτυχία επιλέξτε "Τέλος
	της εγκατάστασης και επανεκκίνηση", βγάλτε το CD σας και περιμένετε μέχρι
	να ξαναξεκινήσει ο υπολογιστής σας. Βεβαιωθείτε ότι εκκινεί από τον σκληρό
	δίσκο, σταυρώστε τα δάχτυλά σας και περιμένετε μέχρι να ξεκινήσει η βασική
	διαμόρφωση.
	
	Η πορεία μέσω της βασικής διαμόρφωσης (base-config) δεν είναι όμως 
	μέσα στους σκοπούς αυτού του κειμένου καθώς δεν αποτελεί μέρος του debian-installer.
	
	4. Αναφορά Εγκατάστασης
	
	
	Αν καταφέρατε με επιτυχία μια εγκατάσταση με τον debian-installer,
	παρακαλούμε να αφιερώσετε λίγο χρόνο για να μας στείλετε μια
	αναφορά. Υπάρχει ένα υπόδειγμα με το όνομα "install-report.template"
	στο φάκελο /root του μόλις εγκατεστημένου συστήματός σας. Παρακαλούμε να
	τη συμπληρώσετε και να τη στείλετε σαν αναφορά σφάλματος για το πακέτο
	"installation-reports". Δείτε την ενότητα 5 σχετικά με τη καταγραφή
	σφαλμάτων.

	
	5. Αναφορά σφαλμάτων
	
	
	Αν δεν φτάσατε στη βασική διαμόρφωση ή συναντήσατε οποιοδήποτε άλλο
	πρόβλημα, πιθανόν να βρήκατε κάποιο σφάλμα στον debian-installer. Για να 
	βελτιώσουμε τον εγκαταστάτη είναι απαραίτητο να ξέρουμε για τέτοια
	σφάλματα, οπότε παρακαλούμε να διαθέσετε λίγο χρόνο για την αναφορά τους.
	
	Πρώτα κοιτάξτε εδώ για να δείτε αν το σφάλμα σας έχει ήδη αναφερθεί:
	http://bugs.qa.debian.org/cgi-bin/debian-installer.cgi?full=yes
	
	Η σελίδα είναι οργανωμένη σύμφωνα με τα πακέτα που αποτελούν
	τα ξεχωριστά υποσυστήματα του debian-installer. Αναφέρετε το σφάλμα
	σας σε σχέση με αντίστοιχο υποσύστημα ή αν δεν ξέρετε σε ποιο ακριβώς
	αντιστοιχεί σε σχέση με το πακέτο "install". Κοιτάξτε εδώ για περισσότερες
	εξηγήσεις όσον αφορά την αναφορά σφαλμάτων:
	
	http://www.debian.org/Bugs/Reporting
	
	
	6. Συμμετάσχετε 
	
	
	Για την Ομάδα του Debian-Installer είναι πάντα ευπρόσδεκτοι όσοι θα
	ήθελαν να δουλέψουν για τον εγκαταστάτη. Έχουμε άφθονη δουλειά να
	κάνουμε: διόρθωση σφαλμάτων, βελτίωση της χρηστικότητας, δημιουργια νέων
	modules και φυσικά εκτεταμένη δοκιμασία. Αν ενδιαφέρεστε να βοηθήσετε ελέγξτε
	αυτή τη σελίδα:

	http://www.debian.org/devel/debian-installer/
	
	
	
	
Συντήρηση: Εμμανουήλ Γαλάτουλας, quad-nrg.net	
15-01-2004
