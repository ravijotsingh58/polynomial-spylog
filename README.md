#### DISCLAIMER : OUR TOOLS ARE FOR EDUCATIONAL PURPOSES ONLY. DON'T USE THEM FOR ILLEGAL ACTIVITIES. YOU ARE THE ONLY RESPONSABLE FOR YOUR ACTIONS! OUR TOOLS ARE OPEN SOURCE WITH NO WARRANTY AND AS ARE.

# polynomial-spylog
Keylogger is 100% invisible keylogger not only for users, but also undetectable by antivirus software. polynomial spylog Monitors all keystokes. It has a seperate process which continues capture system screenshot and send to registered gmail in given time.

#### FEATURES OF POLYNOMIAL SPYLOG
- Discrete/Tamper Proof :By design, Advance Keylogger is undetectable and thus cannot be tampered with or removed by kids/employees .

- Keystrokes Typed: See every keystroke typed even if it is deleted. This keystroke logger feature provides a reader-friendly version of all keystrokes logged along with the raw keylogging activity so you can see every detail.

- Continuous Screenshots: will Capture desktop screen-shots after a given 7 seconds (u are free to change time intetval accoring to requirements).

- Decrypt passwords stored by user in browser and store decrypted passwords in dat file (Newly added feature. Special thanks to EVIL-ACID for awesome tutorial on this. 
  for more info, visit https://github.com/EVIL-ACID/Malware-Development/tree/main/Malware101:Infostealers)

- Email Sending: Screenshot, browser passwords file and keylogger Logfile which contain senstive user information send to registered email (Gmail only) (Mobile/Web/System). Script to send email is also written in C++. 

- AutoStart : Keylogger has functionaility to auto execute on system bootup. It Insert entry on system startup program when it is running.

- AutoCopy : Keylogger has functionaility to auto copy in %appdata%/roaming/Microsoft/CLR (The directory will be created at the time of first time execution).

#### Required Lib:
- CURL
- sqlite (sqlite3.dll)

#### Notes:
- Current version only supports Gmail. So u have to provide your Gmail's Email ID and APP password. You have to generate APP password from GMAIL account's security settings after enabling 2-step-verification on your gmail account. Now a days Gmail has incresed security and due to that, to receive email from 3rd party un-autheticated apps, users has to generate app password and use that to autheticate 3rd party apps. 
- Once app starts executing (and you have provided your creds correctly), you will start receiving emails from your own email that will contains `zip` file contains log data (Screen shots and Key logs file).
