Template: ethdetect/detection_type
Type: select
Choices: passive, none
Choices-nl: passief, geen
Description: What level of hardware detection would you like?
 I can automatically detect some hardware.  If you want me to try to detect
 your hardware you can choose between full and passive detection.  Full
 detection may involve probing which could cause the computer to hang.
 Some hardware, like ISA cards, can only be detected with full detection.
 If you don't want me to do any detection, choose 'none' and you will be
 prompted.
Description-nl: Welk niveau hardware detectie wilt u gebruiken?
 Ik kan een deel van uw hardware automatisch herkennen. Indien u wilt dat
 ik uw hardware ga detecteren, kunt u kiezen uit volledige of passieve
 detectie. Volledige detectie kan leiden tot het vastlopen van uw computer.
 Sommige hardwre, zoals bijv. ISA kaarten, kunnen alleen met volledige
 detectie herkend worden. Indien u niet wilt dat ik aan enige detectie doe,
 kies dan 'geen' en u zal vragen voorgeschoteld krijgen.

Template: ethdetect/module_select
Type: select
Choices: 3c509, ne, ne2000, 3c59x, acenic, dgrs, dmfe, eepro100, epic100, hp100, ibmtr, ne2k-pci, old_tulip, rtl8139, sis900, sktr, tlan, tulip, other
Choices-nl: 3c509, ne, ne2000, 3c59x, acenic, dgrs, dmfe, eepro100, epic100, hp100, ibmtr, ne2k-pci, old_tulip, rtl8139, sis900, sktr, tlan, tulip, other
Description: What module does your ethernet card require?
 This is a list of modules that I know about.  Choose the module from the
 list that supports your card.  If your card requires a different module,
 choose 'other' and you will be prompted for the location of that module.
Description-nl: Welke module gebruikt uw ethernet kaart?
 Dit is een lijst van modules die ik ken. Kies de module uit de lijst
 die uw kaart ondersteunt. Indien uw kaart een andere module nodig
 heeft, kies dan 'other' en u zal gevraagd worden naar de locatie van
 die module.

Template: ethdetect/module_prompt
Type: string
Description: Where is the module for your ethernet card?
 Please enter the full path to the module for your ethernet card.
Description-nl: Waar is de module voor uw ethernet kaart?
 Geef het volledige pad waar de module van uw ethernet kaart staat.

Template: ethdetect/module_params
Type: string
Description: Please enter any additional parameters.
 Some modules accept load-time parameters to customize their operation.
 These parameters are often I/O port and IRQ numbers that vary from machine
 to machine and cannot be determined from the hardware. An example string
 looks  something like "IRQ=7 IO=0x220"
Description-nl: Geef aanvullende parameters
 Sommige modules accepteren parameters bij het laden die hun werking
 kunnen aanpassen. Deze parameters zijn vaak de I/O poort en IRQ nummers
 die per machine verschillend zijn en niet door de hardware bepaald
 kunnen worden. Een voorbeeld van zo'n regel ziet er uit als "IRQ=7
 IO=0x220"

Template: ethdetect/load_module
Type: boolean
Description: Would you like me to attempt to load the '${module}' module?
 An ethernet card has been detected on the ${bus} bus.  In order for the
 operating system to use this card the module '${module}' must be loaded.
Description-nl: Wilt u dat ik probeer de '${module}' module te laden?
 Een ethernet kaart is gevonden op de ${bus} bus. Om deze kaart door het
 besturingssysteem te laten gebruiken moet de '${module}' module geladen
 worden.

Template: ethdetect/error
Type: note
Description: An error occured.
 Something went wrong.
Description-nl: Een foutje is opgetreden
 Er is iets misgegaan.

Template: ethdetect/nothing_detected
Type: note
Description: No ethernet cards were detected.
 Make sure the card is firmly seated and try again.
Description-nl: Geen ethernetkaarten gevonden.
 Controleer of de kaart goed vast zit en probeer 't nog eens.

