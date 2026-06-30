"""Author valid NOVA-8 carts: seed corpus + patch-regression inputs."""
import os, sys, struct
sys.path.insert(0, os.path.join(os.path.dirname(__file__), 'poc'))
import novabuild as nb

ROOT = os.path.join(os.path.dirname(__file__), '..')
CORP = os.path.join(ROOT, 'fuzz', 'corpus')


def sprt():
    b = struct.pack('<HHI', 2, 2, 8)
    for _ in range(2):
        b += struct.pack('<HH', 8, 8) + bytes((i*3) & 0xff for i in range(64))
    return b


def mapc():
    b = struct.pack('<HHHH', 4, 4, 8, 8)
    b += b'\x00' * (4 * 4 * 2)
    return b


def pal():
    b = struct.pack('<HH', 2, 16)
    for i in range(2*16):
        b += struct.pack('<I', 0xff000000 | (i*7 & 0xffffff))
    return b


def snd():
    b = struct.pack('<H', 1)
    b += struct.pack('<Hhhhbb', 4, 0, 3, 2, 1, 1)
    b += struct.pack('<4h', 0, 40, 80, 60)
    return b


def patn():
    b = struct.pack('<HH', 2, 2)
    b += bytes([0, 1])
    for _ in range(2):
        b += struct.pack('<H', 4)
        b += bytes([0]*16)
    return b


def font():
    b = struct.pack('<H', 2)
    b += bytes([4, 4, 0, 0]) + bytes(range(16))
    b += bytes([4, 4, 1, 1]) + bytes([0])
    return b


def data():
    return bytes(((i*5) + 1) & 0xff for i in range(64))


def code_main():
    a = nb.Asm()
    a.emit([nb.BANK, 0x01])
    a.push(10); a.push(42); a.emit([nb.STM])
    a.push(10); a.emit([nb.LDM, nb.POP])
    a.push(2); a.push(3); a.emit([nb.ADD, nb.POP])
    a.call('func', 2)
    a.push(0); a.push(5); a.push(5); a.emit([nb.SPR])
    a.push(0); a.emit([nb.PALSET])
    a.push(0); a.push(7); a.emit([nb.PALCYC])
    a.emit([nb.SPUSH, nb.SREAD, nb.POP, nb.SPOP])
    a.push(0); a.push(0); a.emit([nb.VOICE])
    a.push(0); a.push(60); a.emit([nb.NOTE])
    a.push(0); a.push(1); a.push(1); a.emit([nb.SPAWN, nb.POP])
    a.emit([nb.PLAY, 0x00])
    a.emit([nb.TICK, nb.HALT])
    a.label('func')
    a.emit([nb.LREF, 0x00, nb.LDREF, nb.POP, nb.RET])
    return a.build()


def full_cart():
    return nb.build([
        ('CODE', code_main(), None),
        ('DATA', data(), None),
        ('SPRT', sprt(), None),
        ('MAP ', mapc(), None),
        ('PAL ', pal(), None),
        ('SND ', snd(), None),
        ('PATN', patn(), None),
        ('FONT', font(), None),
    ], entry_pc=0)


def savestate_blob():
    b = struct.pack('<I', 1)
    b += bytes([1]) + struct.pack('<H', 8) + struct.pack('<ii', 0, 60)
    return b


def w(path, data):
    os.makedirs(os.path.dirname(path), exist_ok=True)
    with open(path, 'wb') as f:
        f.write(data)
    print("seed", os.path.relpath(path, ROOT), len(data))


if __name__ == '__main__':
    fc = full_cart()
    w(os.path.join(CORP, 'cart_fuzzer', 'seed_main.bin'), fc)
    w(os.path.join(CORP, 'audio_fuzzer', 'seed_main.bin'), fc)
    w(os.path.join(CORP, 'sprite_fuzzer', 'seed_main.bin'), fc)
    w(os.path.join(CORP, 'font_fuzzer', 'seed_main.bin'), fc)
    w(os.path.join(CORP, 'savestate_fuzzer', 'seed_save.bin'), savestate_blob())

    def s16(v):
        return struct.pack('<h', v)
    gpu = bytes([1, 3])                                        # CLEAR color 3
    gpu += bytes([4]) + s16(2) + s16(2) + s16(20) + s16(10) + bytes([4])   # RECT
    gpu += bytes([7]) + s16(0) + s16(5) + s16(5)              # SPRITE id 0
    gpu += bytes([6]) + s16(40) + s16(40) + s16(8) + bytes([5])           # CIRCLE
    gpu += bytes([0])                                          # END
    w(os.path.join(CORP, 'gpu_fuzzer', 'seed_list.bin'), gpu)
