Template: mirror/country
Type: select
Choices: ${countries}
Default: United States
Description: Use a mirror from what country?
 The goal is to find a mirror that is close to you on the network -- be 
 aware that near countries, or even your own, may not be the best choice.
Description-sv: Använd spegel i vilket land?
 Målet är att hitta en spegel som är nära dig på nätverket -- notera att
 närliggande länder, eller till och med ditt eget, är inte alltid det bästa
 valet.

Template: mirror/protocol
Type: select
Choices: ${protocols}
Default: http
Description: Download files using what protocol?
 Select a protocol to use to download files. If you are unsure,
 choose http as it is less prone to problems involving firewalls.
Description-sv: Vilket protokoll vill du använda för att hämta filer?
 Välj ett protokoll att använda för att hämta filer. Om du är osäker,
 välj http eftersom det har mindre problem med brandväggar.

Template: mirror/http/mirror
Type: select
Choices: ${mirrors}
Default: http.us.debian.org
Description: Choose the Debian mirror to use:
 Select the mirror Debian will be downloaded from. You should select a
 mirror that is close to you on the net.
 .
 Usually, ftp.<your country code>.debian.org is a good choice.
Description-sv: Välj vilkan Debianspegel du vill använda:
 Välj den spegel Debian ska hämtas från. Du bör välja en spegel som
 är så nära dig som möjligt på nätet.
 .
 För det mesta är ftp.<din landskod>.debian.org ett bra val.

Template: mirror/http/hostname
Type: string
Description: Enter mirror hostname:
 Enter the hostname of mirror Debian will be downloaded from.
Description-sv: Ange spegelns värdnamn:
 Ange värdnamnet för spegeln Debian ska hämtas från.

Template: mirror/http/directory
Type: string
Default: /debian
Description: Enter mirror directory:
 Enter the directory the Debian mirror is located in.
Description-sv: Ange spegelns katalog:
 Ange katalogen som Debianspegeln ligger i.

Template: mirror/http/proxy
Type: string
Description: Enter http proxy information, or leave blank for none:
 If you need to use a http proxy to access the outside world, enter the
 proxy information here. Otherwise, leave this blank.
 .
 When entering proxy information, use the standard form of
 "http://[[user][:pass]@]host[:port]/"
Description-sv: Ange proxyinformation, eller lämna tomt:
 Om du måste använda en http-proxy för att komma åt världen utanför, ange
 informationen här. Annars kan du lämna fältet tomt.
 .
 När du anger proxyinformation, använd standardformatet
 "http://[[användare][:lösenord]@]värd[:port]/"

Template: mirror/ftp/mirror
Type: select
Choices: ${mirrors}
Default: ftp.us.debian.org
Description: Choose the Debian mirror to use:
 Select the mirror Debian will be downloaded from. You should select a 
 mirror that is close to you on the net.
 .
 Usually, ftp.<your country code>.debian.org is a good choice.
Description-sv: Välj vilken Debianspegel du vill använda:
 Välj den spegel Debian ska hämtas från. Du bör välja en spegel som
 är så nära dig som möjligt på nätet.
 .
 För det mesta är ftp.<din landskod>.debian.org ett bra val.

Template: mirror/ftp/hostname
Type: string
Description: Enter mirror hostname:
 Enter the hostname of mirror Debian will be downloaded from.
Description-sv: Ange spegelns värdnamn:
 Ange värdnamnet för spegeln Debian ska hämtas från.

Template: mirror/ftp/directory
Type: string
Default: /debian
Description: Enter mirror directory:
 Enter the directory the Debian mirror is located in.
Description-sv: Ange spegelns katalog:
 Ange katalogen som Debianspegeln ligger i.

Template: mirror/ftp/proxy
Type: string
Description: Enter ftp proxy information, or leave blank for none:
 If you need to use a http proxy to access the outside world, enter the
 proxy information here. Otherwise, leave this blank.
 .
 When entering proxy information, use the standard form of
 "ftp://[[user][:pass]@]host[:port]/"
Description-sv: Ange proxyinformation, eller lämna tomt:
 Om du måste använda en ftp-proxy  för att komma åt världen utanför, ange
 informationen här. Annars kan du lämna fältet tomt.
 .
 När du anger proxyinformation, använd standardformtet
 "ftp://[[användare][:lösenord]@]värd[:port]/"


Template: mirror/distribution
Type: select
Choices: woody, sarge, sid
Default: sarge
Description: Select distribution to install
 Please select which of the distributions you want to install.
 .
 woody is the current stable release of Debian
 sarge is the "testing" unreleased version of Debian
 sid is "unstable" and will never be released.
Description-sv: Välj distribution att installera
 Välj vilken av distributionerna du vill installera.
 .
 woody är den nuvarande stabila versionen av Debian.
 sarge är "testing", en icke släppt version av Debian.
 sid är "unstable" och kommer aldrig släppas.
