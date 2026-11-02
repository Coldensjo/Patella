r"""Read a running client's game state and window layout from its memory (issue #19).

Works on the original Tibia.exe (fixed addresses, from the comments in src/) and on
Patella.exe (addresses from its PDB through dbghelp, relative to the module base: Patella.exe
is relocatable). Patella's structures have the original layout (static_asserts in src/), so
the same field offsets serve both.

state(): the client state the server's packets lead to, as a JSON-able dict:
  - connection state, player ID and statistics (Creatures.cpp), the skills;
  - the map centre and every map point's objects (Map.cpp Fields: type, data, data2 of the
    ObjectCount objects; creatures are type 99 with the creature ID);
  - the creature table (ID, name, position, direction, outfit, light, health, speed, skull,
    shield, visible) of the used slots;
  - the body slots, the open containers (object, name, capacity, objects) and both trade
    offers (Inventory.cpp);
  - the default channel's lines (mode and text without the time stamp: Talk.cpp).
  Left out on purpose: times and animation state (step and mark times, effects), which
  depend on the clock, not on the packets.

gui(): the window tree under MainWindow and ModalWindow (TComponent: Parent +0xc, Next +0x10,
OffsetX/Y +0x14/+0x18, Width/Height +0x1c/+0x20; TWindow: FirstChild +0x24) as a list of
(depth, class, x, y, width, height) with client-area coordinates. A component is a window
when its vtable slot +0x20 (GetFirstChild) is `mov eax, [ecx+24h]; ret`; TElement's throws
"not possible". Class names come from Patella's RTTI (Tibia.exe has none for these classes:
there ORIGINAL_VTABLES names the few the session needs, the vtable address stands in for the
rest).
"""

import ctypes
import ctypes.wintypes as wt
import struct

kernel32 = ctypes.WinDLL("kernel32", use_last_error=True)
psapi = ctypes.WinDLL("psapi")
kernel32.OpenProcess.restype = wt.HANDLE
kernel32.OpenProcess.argtypes = [wt.DWORD, wt.BOOL, wt.DWORD]
kernel32.ReadProcessMemory.argtypes = [wt.HANDLE, ctypes.c_void_p, ctypes.c_void_p, ctypes.c_size_t,
	ctypes.POINTER(ctypes.c_size_t)]
kernel32.CloseHandle.argtypes = [wt.HANDLE]
psapi.EnumProcessModulesEx.argtypes = [wt.HANDLE, ctypes.POINTER(ctypes.c_void_p), wt.DWORD,
	ctypes.POINTER(wt.DWORD), wt.DWORD]

PROCESS_QUERY_INFORMATION = 0x0400
PROCESS_VM_READ = 0x0010

# Tibia.exe: the globals' addresses (comments in src/*.cpp).
ORIGINAL = {
	"ConnectionState": 0x71c588,
	"ModalWindow": 0x5d16ac,
	"MainWindow": 0x5d16b0,
	"SkinProperties": 0x5ccfc8,
	"PlayerState": 0x5c67d8,
	"PlayerSkillProgress": 0x5c67dc,
	"PlayerSkillValue": 0x5c67f8,
	"FollowID": 0x5c6818,
	"AttackID": 0x5c681c,
	"PlayerCapacity": 0x5c6820,
	"PlayerSoul": 0x5c6824,
	"PlayerMaxMana": 0x5c6828,
	"PlayerMana": 0x5c682c,
	"PlayerMagicLevelProgress": 0x5c6830,
	"PlayerLevelProgress": 0x5c6834,
	"PlayerMagicLevel": 0x5c6838,
	"PlayerLevel": 0x5c683c,
	"PlayerExperience": 0x5c6840,
	"PlayerMaxHealth": 0x5c6844,
	"PlayerHealth": 0x5c6848,
	"PlayerID": 0x5c684c,
	"Creatures": 0x5c68b0,
	"Body": 0x5ced60,
	"Containers": 0x5cedd8,
	"OwnOffer": 0x5d0c98,
	"CounterOffer": 0x5d1170,
	"MapZ": 0x5d16e8,
	"MapY": 0x5d16ec,
	"MapX": 0x5d16f0,
	"Fields": 0x5d4c18,
	"DefaultTextBuffer": 0x71ef68,
	"Characters": 0x71c54c,
	"AccountCharacters": 0x71c550,
}

# Tibia.exe has no RTTI for its windows: the vtables of the classes the session driver looks
# for (comments in src/*.h, or the vtable store in the constructor).
ORIGINAL_VTABLES = {
	0x56cf38: "TMapWindow",            # MapWindow.h
	0x55ed38: "TInventoryView",        # constructor 0x42a750
	0x55eed0: "TContainerWindowBody",  # constructor 0x438500
	0x55f138: "TTradeOffers",          # constructor 0x430dc0
	0x55f2a0: "TTradeButtons",         # constructor 0x432f00
	0x55f370: "TTradeWindowBody",      # constructor 0x43baf0
	0x55f730: "TContextMenu",          # ContextMenus.h
	0x55f6a8: "TContextMenuEntry",     # ContextMenus.h
	0x560bb8: "TLabel",                # DialogElements.h
	0x561c40: "TTextButton",           # DialogElements.h
	0x563778: "TDialog",               # Dialogs.h
	# issue #21 (tools/compare_files.py): the option and mark dialogs, their controls, the automap
	# and the console's tabs
	0x56c5c0: "TStartWindow",          # MainWindow.h
	0x563258: "TOptionsDialog",        # Dialogs.h
	0x563360: "TGeneralOptionsDialog", # Dialogs.h
	0x5624b8: "TGraphicsOptionsDialog",  # Dialogs.h
	0x562738: "TAdvancedGraphicsDialog",  # Dialogs.h
	0x563598: "TConsoleOptionsDialog",  # Dialogs.h
	0x563650: "TNetworkOptionsDialog",  # Dialogs.h
	0x563a90: "THotkeyDialog",         # Dialogs.h
	0x562698: "TEditMarkDialog",       # Dialogs.h
	0x563b30: "TErrorReportDialog",    # Dialogs.h
	0x561de8: "TCheckBox",             # DialogElements.h
	0x55e688: "TAutomapView",          # ContentWindows.h (notes/decisions.md, ContentWindows.cpp)
	0x55ea08: "TAutomapWindow",        # ContentWindows.h
	0x55e098: "TTab",                  # ConsoleWindow.h
}

SKILLS = 7
CREATURES = 150
CREATURE_SIZE = 0x9c
BODY_POSITIONS = 10
OPEN_CONTAINERS = 16
CONTAINER_SIZE = 0x1ec
MAP_POINTS = 18 * 14 * 8
MAP_POINT_SIZE = 0xac
WINDOW_GETFIRSTCHILD = bytes.fromhex("8b4124c3")


class ReadError(Exception):
	pass


def pdb_symbols(exe_path, names):
	"""RVA of each name in the PDB of exe_path (dbghelp finds it through the debug directory)."""
	dbghelp = ctypes.WinDLL("dbghelp")

	class SYMBOL_INFO(ctypes.Structure):
		_fields_ = [("SizeOfStruct", wt.ULONG), ("TypeIndex", wt.ULONG), ("Reserved", ctypes.c_uint64 * 2),
			("Index", wt.ULONG), ("Size", wt.ULONG), ("ModBase", ctypes.c_uint64), ("Flags", wt.ULONG),
			("Value", ctypes.c_uint64), ("Address", ctypes.c_uint64), ("Register", wt.ULONG),
			("Scope", wt.ULONG), ("Tag", wt.ULONG), ("NameLen", wt.ULONG), ("MaxNameLen", wt.ULONG),
			("Name", ctypes.c_char * 256)]

	ENUMPROC = ctypes.WINFUNCTYPE(wt.BOOL, ctypes.POINTER(SYMBOL_INFO), wt.ULONG, ctypes.c_void_p)
	dbghelp.SymInitialize.argtypes = [wt.HANDLE, ctypes.c_char_p, wt.BOOL]
	dbghelp.SymLoadModuleEx.restype = ctypes.c_uint64
	dbghelp.SymLoadModuleEx.argtypes = [wt.HANDLE, wt.HANDLE, ctypes.c_char_p, ctypes.c_char_p, ctypes.c_uint64,
		wt.DWORD, ctypes.c_void_p, wt.DWORD]
	dbghelp.SymEnumSymbols.argtypes = [wt.HANDLE, ctypes.c_uint64, ctypes.c_char_p, ENUMPROC, ctypes.c_void_p]
	dbghelp.SymCleanup.argtypes = [wt.HANDLE]
	handle = wt.HANDLE(0x7919)
	dbghelp.SymSetOptions(0x2 | 0x4)  # SYMOPT_UNDNAME | SYMOPT_DEFERRED_LOADS
	if not dbghelp.SymInitialize(handle, None, False):
		raise ReadError("SymInitialize failed")
	try:
		base = dbghelp.SymLoadModuleEx(handle, None, exe_path.encode(), None, 0x400000, 0, None, 0)
		if not base:
			raise ReadError("SymLoadModuleEx failed for %s" % exe_path)
		result = {}
		for name in names:
			hits = []

			def callback(info, size, context):
				if info.contents.Name.decode() == name:
					hits.append(info.contents.Address)
				return True

			dbghelp.SymEnumSymbols(handle, base, name.encode(), ENUMPROC(callback), None)
			if len(set(hits)) != 1:
				raise ReadError("%s: %d symbols named %s" % (exe_path, len(set(hits)), name))
			result[name] = hits[0] - base
		return result
	finally:
		dbghelp.SymCleanup(handle)


class Client:
	"""A running client's memory. symbols: None for Tibia.exe, else the exe whose PDB to use."""

	def __init__(self, pid, pdb_exe=None):
		self.handle = kernel32.OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, False, pid)
		if not self.handle:
			raise ReadError("OpenProcess %d failed (error %d)" % (pid, ctypes.get_last_error()))
		self.base = self.module_base()
		if pdb_exe is None:
			self.addr = dict(ORIGINAL)
			self.rtti = False
		else:
			rvas = pdb_symbols(pdb_exe, list(ORIGINAL))
			self.addr = {name: self.base + rva for name, rva in rvas.items()}
			self.rtti = True

	def close(self):
		if self.handle:
			kernel32.CloseHandle(self.handle)
			self.handle = None

	def module_base(self):
		modules = (ctypes.c_void_p * 256)()
		need = wt.DWORD()
		if not psapi.EnumProcessModulesEx(self.handle, modules, ctypes.sizeof(modules), ctypes.byref(need), 1):
			raise ReadError("EnumProcessModulesEx failed")
		return modules[0]

	def read(self, address, count):
		buffer = ctypes.create_string_buffer(count)
		got = ctypes.c_size_t()
		if not kernel32.ReadProcessMemory(self.handle, address, buffer, count, ctypes.byref(got)) \
				or got.value != count:
			raise ReadError("ReadProcessMemory 0x%x (%d bytes) failed" % (address, count))
		return buffer.raw

	def u32(self, address):
		return struct.unpack("<I", self.read(address, 4))[0]

	def i32(self, address):
		return struct.unpack("<i", self.read(address, 4))[0]

	def cstr(self, address, limit=256):
		# up to the end of the page first: a short string near the end of the last committed
		# page of a heap region cannot be read with `limit` bytes
		first = min(limit, 0x1000 - (address & 0xfff))
		data = self.read(address, first)
		if b"\0" not in data and first < limit:
			data += self.read(address + first, limit - first)
		return data.split(b"\0", 1)[0].decode("latin-1")

	def var(self, name):
		return self.i32(self.addr[name])

	# --- state ---------------------------------------------------------------------------

	def state(self):
		s = {}
		s["connection"] = self.var("ConnectionState")
		s["player"] = {name: self.var(name) for name in ("PlayerID", "PlayerHealth", "PlayerMaxHealth",
			"PlayerExperience", "PlayerLevel", "PlayerLevelProgress", "PlayerMagicLevel",
			"PlayerMagicLevelProgress", "PlayerMana", "PlayerMaxMana", "PlayerSoul", "PlayerCapacity",
			"PlayerState", "AttackID", "FollowID")}
		s["skills"] = list(struct.unpack("<%di" % SKILLS, self.read(self.addr["PlayerSkillValue"], 4 * SKILLS)))
		s["skill_progress"] = list(struct.unpack("<%di" % SKILLS,
			self.read(self.addr["PlayerSkillProgress"], 4 * SKILLS)))
		s["map_centre"] = [self.var("MapX"), self.var("MapY"), self.var("MapZ")]
		s["map"] = self.map_points()
		s["creatures"] = self.creatures()
		s["body"] = [list(struct.unpack_from("<3i", self.read(self.addr["Body"], 12 * BODY_POSITIONS), 12 * i))
			for i in range(BODY_POSITIONS)]
		s["containers"] = self.containers()
		s["trade"] = [self.offer(self.addr["OwnOffer"]), self.offer(self.addr["CounterOffer"])]
		s["console"] = self.console()
		return s

	def map_points(self):
		minimum, count, entry = struct.unpack("<iiI", self.read(self.addr["Fields"], 12))
		if count != MAP_POINTS or entry == 0:
			raise ReadError("Fields: min %d, %d entries at 0x%x" % (minimum, count, entry))
		data = self.read(entry, MAP_POINTS * MAP_POINT_SIZE)
		points = []
		for i in range(MAP_POINTS):
			offset = i * MAP_POINT_SIZE
			objects = struct.unpack_from("<i", data, offset)[0]
			if objects < 0 or objects > 10:
				raise ReadError("map point %d: %d objects" % (i, objects))
			points.append([list(struct.unpack_from("<3i", data, offset + 4 + 12 * j)) for j in range(objects)])
		return points

	def creatures(self):
		data = self.read(self.addr["Creatures"], CREATURES * CREATURE_SIZE)
		out = []
		for i in range(CREATURES):
			c = data[i * CREATURE_SIZE:(i + 1) * CREATURE_SIZE]
			creature_id = struct.unpack_from("<I", c, 0)[0]
			if creature_id == 0:
				continue
			out.append({"slot": i, "id": creature_id,
				"name": c[4:36].split(b"\0", 1)[0].decode("latin-1"),
				"pos": list(struct.unpack_from("<3i", c, 0x24)),
				"direction": struct.unpack_from("<i", c, 0x50)[0],
				"outfit": list(struct.unpack_from("<5i", c, 0x60)),
				"light": list(struct.unpack_from("<2i", c, 0x74)),
				"mark": struct.unpack_from("<i", c, 0x7c)[0],
				"health": struct.unpack_from("<i", c, 0x84)[0],
				"speed": struct.unpack_from("<i", c, 0x88)[0],
				"visible": c[0x8c],
				"skull": struct.unpack_from("<i", c, 0x90)[0],
				"shield": struct.unpack_from("<i", c, 0x94)[0]})
		return out

	def containers(self):
		data = self.read(self.addr["Containers"], OPEN_CONTAINERS * CONTAINER_SIZE)
		out = []
		for i in range(OPEN_CONTAINERS):
			c = data[i * CONTAINER_SIZE:(i + 1) * CONTAINER_SIZE]
			if not c[0]:
				continue
			count = struct.unpack_from("<i", c, 0x38)[0]
			out.append({"index": i, "object": list(struct.unpack_from("<3i", c, 4)),
				"name": c[0x10:0x2e].split(b"\0", 1)[0].decode("latin-1"),
				"capacity": struct.unpack_from("<i", c, 0x30)[0], "has_parent": c[0x34],
				"objects": [list(struct.unpack_from("<3i", c, 0x3c + 12 * j)) for j in range(max(0, min(count, 36)))]})
		return out

	def offer(self, address):
		head = self.read(address, 0x24)
		count = struct.unpack_from("<i", head, 0x20)[0]
		count = max(0, min(count, 100))
		objects = self.read(address + 0x24, 12 * count) if count else b""
		return {"name": head[:30].split(b"\0", 1)[0].decode("latin-1"), "count": count,
			"objects": [list(struct.unpack_from("<3i", objects, 12 * j)) for j in range(count)]}

	def console(self):
		buffer = self.u32(self.addr["DefaultTextBuffer"])
		if buffer == 0:
			return []
		line = self.u32(buffer + 0x10)
		out = []
		while line and len(out) < 2000:
			mode = self.i32(line + 8)
			text = self.u32(line + 0x34)
			out.append([mode, self.cstr(text, 512) if text else None])
			line = self.u32(line + 0x10)
		return out

	def characters(self):
		"""The names in the character list (Status.cpp: TCharacter, 0x54 bytes)."""
		count = self.var("AccountCharacters")
		array = self.u32(self.addr["Characters"])
		if count <= 0 or array == 0:
			return []
		return [self.cstr(array + 0x54 * i, 30) for i in range(count)]

	# --- window tree ------------------------------------------------------------------------

	def class_name(self, vtable):
		if self.rtti:
			try:
				locator = self.u32(vtable - 4)
				descriptor = self.u32(locator + 12)
				name = self.cstr(descriptor + 8, 128)
				if name.startswith(".?AV") and name.endswith("@@"):
					return name[4:-2]
			except ReadError:
				pass
		original = vtable - self.base + 0x400000
		return ORIGINAL_VTABLES.get(original, "vt_%x" % original) if not self.rtti else "vt_%x" % original

	def is_window(self, vtable):
		function = self.u32(vtable + 0x20)
		return self.read(function, 4) == WINDOW_GETFIRSTCHILD

	def gui(self):
		nodes = []

		def walk(component, depth, x, y):
			count = 0
			while component and count < 500:
				vtable = self.u32(component)
				ox, oy, width, height = struct.unpack("<4i", self.read(component + 0x14, 16))
				node = {"depth": depth, "class": self.class_name(vtable), "x": x + ox, "y": y + oy,
					"w": width, "h": height, "ptr": component, "vtable": vtable}
				if node["class"] == "TLabel":
					text = self.u32(component + 0x28)
					node["text"] = self.cstr(text, 256) if text else None
				elif node["class"] == "TContextMenuEntry":
					node["text"] = self.cstr(component + 0x24, 50)
				elif node["class"] == "TTextButton":  # Text[50] at +0x44 (DialogElements.h)
					node["text"] = self.cstr(component + 0x44, 50)
				nodes.append(node)
				if self.is_window(vtable):
					walk(self.u32(component + 0x24), depth + 1, x + ox, y + oy)
				component = self.u32(component + 0x10)
				count += 1

		main = self.u32(self.addr["MainWindow"])
		if main:
			walk(main, 0, 0, 0)
		modal = self.u32(self.addr["ModalWindow"])
		if modal:
			nodes.append({"depth": 0, "class": "(modal)", "x": 0, "y": 0, "w": 0, "h": 0, "ptr": 0, "vtable": 0})
			walk(modal, 1, 0, 0)
		return nodes

	def skin_rects(self, offset, count):
		"""count TRects (x, y, width, height) at SkinProperties + offset."""
		data = self.read(self.addr["SkinProperties"] + offset, 16 * count)
		return [struct.unpack_from("<4i", data, 16 * i) for i in range(count)]

	def skin_int(self, offset):
		return self.i32(self.addr["SkinProperties"] + offset)
