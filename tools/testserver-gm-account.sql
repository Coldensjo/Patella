-- GameMaster test account for the local 7.72 server (notes/testserver.md).
-- Account 772002, password "gamemaster", premium, one character "GM Patella" on world 1 (Zanera)
-- with every right the shipped GameMasters on account 666666 have (copied from "GM Havey on Zanera").
-- Auth is plain text, as in testserver-account.sql. Rights are read at login, so this can run while
-- the server is up:
--   sqlite3 C:\Tibia\gameserver\tibia.db < tools\testserver-gm-account.sql

BEGIN;

INSERT INTO Accounts (AccountID, Email, Auth, PremiumEnd)
	VALUES (772002, 'gm@test.local', CAST('gamemaster' AS BLOB), 2147483647);

INSERT INTO Characters (WorldID, CharacterID, AccountID, Name, Sex)
	VALUES (1, 15146576, 772002, 'GM Patella', 1);

INSERT INTO CharacterRights (CharacterID, Name)
	SELECT 15146576, r.Name FROM CharacterRights r JOIN Characters c USING (CharacterID)
	WHERE c.Name = 'GM Havey on Zanera';

COMMIT;
