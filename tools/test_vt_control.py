"""
Tests for vt_control.py against a fake terminal: a socket server on 127.0.0.1 that answers the
control interface's line protocol from a made-up screen.

  python -m unittest discover -s tools -p "test_*.py"
"""

import contextlib
import io
import json
import socket
import socketserver
import sys
import threading
import unittest
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))

import vt_control  # noqa: E402

SCREEN = {
    "working_set": 38, "mask": {"id": 1000, "type": "DataMask"}, "soft_key_mask": 4000, "focus": None,
    "objects": [
        {"id": 11000, "type": "OutputString", "text": "Section control", "x": 10, "y": 5, "depth": 1, "w": 200, "h": 20},
        {"id": 12000, "type": "OutputNumber", "text": "12.5", "value": 12.5, "raw": 125, "x": 10, "y": 30,
         "depth": 1, "w": 60, "h": 20},
        {"id": 6000, "type": "Button", "text": "Save", "x": 10, "y": 60, "depth": 1, "w": 80, "h": 40, "enabled": True},
        {"id": 6001, "type": "Button", "text": "Save as", "x": 100, "y": 60, "depth": 1, "w": 80, "h": 40, "enabled": False},
    ],
    "soft_keys": [
        {"id": 5000, "type": "Key", "text": "Home", "pictures": [], "position": 1, "row": 1, "column": 2},
        {"id": 5001, "type": "Key", "text": "", "pictures": [20000], "position": 2, "row": 2, "column": 2},
    ],
}


class FakeTerminal(socketserver.ThreadingTCPServer):
    daemon_threads = True
    allow_reuse_address = False

    def __init__(self):
        super().__init__(("127.0.0.1", 0), Handler)
        self.requests = []
        self.mask = 1000


class Handler(socketserver.StreamRequestHandler):
    def handle(self):
        for line in self.rfile:
            request = json.loads(line)
            self.server.requests.append(request)
            op = request["op"]
            result, error = None, None
            if op == "hello":
                result = {"app": "AgISOVirtualTerminal", "control_protocol": 1, "working_sets": []}
            elif op == "screen":
                result = SCREEN
            elif op == "state":
                result = {**{k: SCREEN[k] for k in ("working_set", "soft_key_mask", "focus")},
                          "mask": {"id": self.server.mask, "type": "DataMask"}}
            elif op in ("softkey", "button"):
                result = {"object_id": request.get("object_id", 5000), "key_code": 1, "mask": 1000}
                self.server.mask = 1001
            elif op == "set_input":
                error = "object 9 is not on the active mask" if request["object_id"] == 9 else None
                result = {"id": request["object_id"], "type": "InputNumber", "value": request.get("value")}
            else:
                error = f"unknown op '{op}'"
            reply = {"id": request["id"], "ok": error is None, **({"error": error} if error else {"result": result})}
            self.wfile.write((json.dumps(reply) + "\n").encode())
            self.wfile.flush()


class VtControlTest(unittest.TestCase):
    def setUp(self):
        self.terminal = FakeTerminal()
        threading.Thread(target=self.terminal.serve_forever, daemon=True).start()
        self.port = self.terminal.server_address[1]

    def tearDown(self):
        self.terminal.shutdown()
        self.terminal.server_close()

    def main(self, *argv):
        out, err = io.StringIO(), io.StringIO()
        with contextlib.redirect_stdout(out), contextlib.redirect_stderr(err):
            code = vt_control.main(["--port", str(self.port), *argv])
        return code, out.getvalue(), err.getvalue()

    def test_a_soft_key_and_a_button_by_their_text(self):
        with vt_control.VtControl(self.port) as vt:
            vt.softkey(text="home")
            vt.button(text="Save")                      # the exact text wins over "Save as"
        presses = [r for r in self.terminal.requests if r["op"] in ("softkey", "button")]
        self.assertEqual([(r["op"], r["object_id"], r["hold_ms"]) for r in presses],
                         [("softkey", 5000, 150), ("button", 6000, 150)])

    def test_a_text_on_several_or_no_keys_is_refused(self):
        with vt_control.VtControl(self.port) as vt:
            with self.assertRaisesRegex(vt_control.VtRefused, "0 soft keys show 'Stop'"):
                vt.softkey(text="Stop")
            with self.assertRaisesRegex(vt_control.VtRefused, "2 buttons show 'sav'"):
                vt.button(text="sav")

    def test_a_soft_key_by_position(self):
        self.assertEqual(self.main("softkey", "--position", "2", "--hold", "300")[0], 0)
        self.assertEqual(self.terminal.requests[-1], {"id": 1, "op": "softkey", "hold_ms": 300, "position": 2})

    def test_wait_for_a_mask_and_a_text(self):
        with vt_control.VtControl(self.port) as vt:
            self.assertTrue(vt.wait_text("section", timeout_s=1)["met"])
            self.assertFalse(vt.wait_mask(1001, timeout_s=0.2)["met"])
            vt.softkey(position=1)
            self.assertTrue(vt.wait_mask(1001, timeout_s=1)["met"])

    def test_the_screen_as_text(self):
        code, out, _ = self.main("screen")
        self.assertEqual(code, 0)
        self.assertIn("mask 1000 (DataMask), soft key mask 4000", out)
        self.assertIn("12000 OutputNumber", out)
        self.assertIn("(disabled)", out)
        self.assertIn("soft key  2 (row 2, column 2): 5001 ''  pictures [20000]", out)

    def test_exit_codes(self):
        self.assertEqual(self.main("set", "12", "--value", "3.5")[0], 0)
        code, _, err = self.main("set", "9", "--value", "1")
        self.assertEqual((code, "not on the active mask" in err), (1, True))
        self.assertEqual(self.main("wait-text", "nothing like it", "--timeout", "0.1")[0], 1)
        self.assertEqual(self.main("raw", '{"op": "fly"}')[0], 1)
        self.assertEqual(self.main("softkey")[0], 1)

    def test_no_terminal(self):
        with socket.socket() as bound:
            bound.bind(("127.0.0.1", 0))
            err = io.StringIO()
            with contextlib.redirect_stderr(err):
                code = vt_control.main(["--port", str(bound.getsockname()[1]), "hello"])
        self.assertEqual((code, "no terminal listens" in err.getvalue()), (2, True))


if __name__ == "__main__":
    unittest.main()
