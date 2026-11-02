-- Test account for the local 7.72 server (issue #18, notes/testserver.md).
-- Account 772001, password "patella", two characters on world 1 (Zanera).
-- Auth holds the password as plain text, one of the forms the server's DB_VerifyPassword
-- (src/database/db_common.hh) accepts. CipSoft's form, SHA256(SHA256(password) XOR salt)
-- followed by the salt, as in schema.sql's sample account, was tried first and refused
-- ("Password is not correct."): that path does not work in this build, whose 111111/tibia
-- passes only through a special case for its first four bytes.
-- Run it once, with the server stopped:
--   sqlite3 C:\Tibia\gameserver\tibia.db < tools\testserver-account.sql
-- The CharacterIDs follow the highest one in the shipped tibia.db (15146573).

BEGIN;

INSERT INTO Accounts (AccountID, Email, Auth)
	VALUES (772001, 'patella@test.local', CAST('patella' AS BLOB));

INSERT INTO Characters (WorldID, CharacterID, AccountID, Name, Sex)
	VALUES (1, 15146574, 772001, 'Patella Tester', 1), (1, 15146575, 772001, 'Patella Second', 1);

COMMIT;
