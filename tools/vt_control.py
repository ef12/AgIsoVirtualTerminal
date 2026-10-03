#!/usr/bin/env python3
"""
Command line and Python client of AgIsoVirtualTerminal's control interface: read what the terminal
shows and press soft keys and buttons and enter values as an operator does, from a test script.

Start the terminal with the interface on:
  AgISOVirtualTerminal.exe --control-port=9300

  vt_control.py [--port N] <command> ...
    hello                              the build, the VT number, whether CAN runs, the working sets
    working-sets                       index, address, NAME, pool state and which one is active
    select INDEX                       make that working set the active one
    state                              the active mask, soft key mask and focused object
    screen [--all] [--hidden]          the objects on the active mask, with text or value, and the soft keys
    object ID                          one object of the active pool (also variables), with its children
    find TEXT                          the objects and soft keys on the screen whose text holds TEXT
    softkey (ID | --position N | --text T) [--hold MS]
    button (ID | --text T) [--hold MS]
    set ID (--value V | --raw R | --index I | --text T)
    screenshot PATH                    the data mask and soft keys as a PNG
    wait-mask ID [--timeout S]         until the active mask is ID
    wait-text TEXT [--timeout S]       until an object on the screen shows TEXT
    raw JSON                           a request as it is, e.g. '{"op": "hello"}'

The port: --port, else $VT_CONTROL_PORT, else 9300. Exit code: 0 done; 1 refused, or a wait not
met; 2 no terminal listening. Standard library only; ControlClient works from any script:

    from vt_control import VtControl
    with VtControl(9300) as vt:
        vt.softkey(text="Save")
        vt.wait_text("Saved", timeout_s=5)
"""

from __future__ import annotations

import argparse
import json
import os
import socket
import sys
import time

HOST = "127.0.0.1"
DEFAULT_PORT = 9300
PORT_ENV = "VT_CONTROL_PORT"
POLL_S = 0.1


class VtRefused(Exception):
    """The terminal answered the request with an error."""


class VtUnavailable(Exception):
    """No terminal listens on the port, or the connection broke."""


class VtControl:
    """One connection to the terminal's control interface."""

    def __init__(self, port: int = DEFAULT_PORT, host: str = HOST, timeout_s: float = 20.0):
        self.address = (host, port)
        self.timeout_s = timeout_s
        self._sock = None
        self._file = None
        self._id = 0

    def __enter__(self) -> VtControl:
        self.connect()
        return self

    def __exit__(self, *exc) -> None:
        self.close()

    def connect(self) -> None:
        try:
            self._sock = socket.create_connection(self.address, timeout=5.0)
        except OSError as exc:
            raise VtUnavailable(f"no terminal listens on {self.address[0]}:{self.address[1]} ({exc}); "
                                "start it with --control-port") from None
        self._sock.settimeout(self.timeout_s)
        self._file = self._sock.makefile("rwb")

    def close(self) -> None:
        for handle in (self._file, self._sock):
            if handle is not None:
                handle.close()
        self._file = self._sock = None

    def request(self, request: dict) -> dict:
        """Send one request ("id" added) and return the reply as it is."""
        if self._sock is None:
            self.connect()
        self._id += 1
        try:
            self._file.write((json.dumps({"id": self._id, **request}) + "\n").encode("utf-8"))
            self._file.flush()
            line = self._file.readline()
        except OSError as exc:
            raise VtUnavailable(f"the connection to the terminal broke ({exc})") from None
        if not line:
            raise VtUnavailable("the terminal closed the connection")
        return json.loads(line)

    def call(self, op: str, **args):
        reply = self.request({"op": op, **args})
        if not reply.get("ok"):
            raise VtRefused(reply.get("error", "refused"))
        return reply.get("result")

    # --- reading
    def hello(self) -> dict:
        return self.call("hello")

    def state(self) -> dict:
        return self.call("state")

    def screen(self, all_objects: bool = False, hidden: bool = False) -> dict:
        return self.call("screen", all=all_objects, hidden=hidden)

    def object(self, object_id: int) -> dict:
        return self.call("object", object_id=object_id)

    def find(self, text: str) -> list[dict]:
        """The objects and soft keys on the screen whose text holds text (case ignored)."""
        screen = self.screen()
        wanted = text.casefold()
        found = [o for o in screen["objects"] if wanted in str(o.get("text", "")).casefold()]
        return found + [k for k in screen["soft_keys"] if wanted in str(k.get("text", "")).casefold()]

    # --- acting
    def softkey(self, object_id: int | None = None, position: int | None = None, text: str | None = None,
                hold_ms: int = 150) -> dict:
        if text is not None:
            object_id = self._one(self.screen()["soft_keys"], text, "soft key")["id"]
        args = {"object_id": object_id} if object_id is not None else {"position": position}
        return self.call("softkey", hold_ms=hold_ms, **args)

    def button(self, object_id: int | None = None, text: str | None = None, hold_ms: int = 150) -> dict:
        if text is not None:
            buttons = [o for o in self.screen()["objects"] if o["type"] == "Button"]
            object_id = self._one(buttons, text, "button")["id"]
        return self.call("button", object_id=object_id, hold_ms=hold_ms)

    def set_input(self, object_id: int, **value) -> dict:
        """value=... (as displayed) or raw=... for a number, value=0/1 for a boolean, index=... for
        a list, text=... for a string."""
        return self.call("set_input", object_id=object_id, **value)

    def screenshot(self, path: str) -> dict:
        return self.call("screenshot", path=os.path.abspath(path))

    def wait_mask(self, mask_id: int, timeout_s: float = 10.0) -> dict:
        return self._wait(lambda: (self.state().get("mask") or {}).get("id") == mask_id, timeout_s)

    def wait_text(self, text: str, timeout_s: float = 10.0) -> dict:
        return self._wait(lambda: bool(self.find(text)), timeout_s)

    def _wait(self, condition, timeout_s: float) -> dict:
        start = time.monotonic()
        while True:
            met = condition()
            elapsed = time.monotonic() - start
            if met or elapsed >= timeout_s:
                return {"met": met, "elapsed_s": round(elapsed, 2), "state": self.state()}
            time.sleep(POLL_S)

    @staticmethod
    def _one(candidates: list[dict], text: str, what: str) -> dict:
        wanted = text.casefold()
        exact = [c for c in candidates if str(c.get("text", "")).casefold() == wanted]
        found = exact or [c for c in candidates if wanted in str(c.get("text", "")).casefold()]
        if len(found) != 1:
            names = ", ".join(f"{c['id']} {c.get('text', '')!r}" for c in found) or "none"
            raise VtRefused(f"{len(found)} {what}s show {text!r}: {names}; give the object ID")
        return found[0]


# ---------------------------------------------------------------------------
#  The command line
# ---------------------------------------------------------------------------

def build_parser() -> argparse.ArgumentParser:
    parser = argparse.ArgumentParser(description="Reads and drives a running AgIsoVirtualTerminal "
                                                 "through its control interface (--control-port).")
    parser.add_argument("--port", type=int, help=f"(default: ${PORT_ENV}, else {DEFAULT_PORT})")
    parser.add_argument("--host", default=HOST)
    parser.add_argument("--json", action="store_true", help="print the result as JSON")
    commands = parser.add_subparsers(dest="command", required=True, metavar="<command>")
    commands.add_parser("hello")
    commands.add_parser("working-sets")
    select = commands.add_parser("select")
    select.add_argument("index", type=int)
    commands.add_parser("state")
    screen = commands.add_parser("screen")
    screen.add_argument("--all", action="store_true", help="also lines, rectangles, ellipses, polygons")
    screen.add_argument("--hidden", action="store_true", help="also the objects of hidden containers")
    obj = commands.add_parser("object")
    obj.add_argument("object_id", type=int)
    find = commands.add_parser("find")
    find.add_argument("text")
    for name in ("softkey", "button"):
        press = commands.add_parser(name)
        press.add_argument("object_id", type=int, nargs="?")
        if name == "softkey":
            press.add_argument("--position", type=int)
        press.add_argument("--text")
        press.add_argument("--hold", type=int, default=150, metavar="MS")
    set_ = commands.add_parser("set")
    set_.add_argument("object_id", type=int)
    value = set_.add_mutually_exclusive_group(required=True)
    value.add_argument("--value", type=float, help="an input number as displayed, or 0/1 for a boolean")
    value.add_argument("--raw", type=int, help="an input number's raw value")
    value.add_argument("--index", type=int, help="an input list's item")
    value.add_argument("--text", help="an input string's text")
    shot = commands.add_parser("screenshot")
    shot.add_argument("path")
    wait_mask = commands.add_parser("wait-mask")
    wait_mask.add_argument("mask_id", type=int)
    wait_mask.add_argument("--timeout", type=float, default=10.0)
    wait_text = commands.add_parser("wait-text")
    wait_text.add_argument("text")
    wait_text.add_argument("--timeout", type=float, default=10.0)
    raw = commands.add_parser("raw")
    raw.add_argument("request_json")
    return parser


def run(vt: VtControl, args):
    """The result of a parsed command line."""
    command = args.command
    if command == "hello":
        return vt.hello()
    if command == "working-sets":
        return vt.call("working_sets")
    if command == "select":
        return vt.call("select_working_set", index=args.index)
    if command == "state":
        return vt.state()
    if command == "screen":
        return vt.screen(args.all, args.hidden)
    if command == "object":
        return vt.object(args.object_id)
    if command == "find":
        return vt.find(args.text)
    if command in ("softkey", "button"):
        position = getattr(args, "position", None)
        if args.object_id is None and args.text is None and position is None:
            raise VtRefused(f"{command} takes an object ID, --text" + (" or --position" if command == "softkey" else ""))
        if command == "softkey":
            return vt.softkey(args.object_id, position, args.text, args.hold)
        return vt.button(args.object_id, args.text, args.hold)
    if command == "set":
        value = {name: getattr(args, name) for name in ("value", "raw", "index", "text") if getattr(args, name) is not None}
        return vt.set_input(args.object_id, **value)
    if command == "screenshot":
        return vt.screenshot(args.path)
    if command == "wait-mask":
        return vt.wait_mask(args.mask_id, args.timeout)
    if command == "wait-text":
        return vt.wait_text(args.text, args.timeout)
    try:
        request = json.loads(args.request_json)
    except json.JSONDecodeError as exc:
        raise VtRefused(f"not JSON: {exc}") from None
    reply = vt.request(request)
    if not reply.get("ok"):
        raise VtRefused(reply.get("error"))
    return reply.get("result")


def screen_lines(screen: dict) -> list[str]:
    mask = screen.get("mask") or {}
    lines = [f"mask {mask.get('id')} ({mask.get('type')}), soft key mask {screen.get('soft_key_mask')}, "
             f"focus {screen.get('focus')}"]
    for item in screen.get("objects", []):
        shown = item.get("text", item.get("value", ""))
        flags = "" if item.get("enabled", True) else "  (disabled)"
        lines.append(f"  {'  ' * max(0, item['depth'] - 1)}{item['id']:>5} {item['type']:<20} "
                     f"@{item['x']},{item['y']}  {shown}{flags}")
    for key in screen.get("soft_keys", []):
        pictures = f"  pictures {key['pictures']}" if key.get("pictures") else ""
        lines.append(f"  soft key {key['position']:>2} (row {key['row']}, column {key['column']}): "
                     f"{key['id']} {key.get('text', '')!r}{pictures}")
    return lines


def main(argv=None) -> int:
    args = build_parser().parse_args(argv)
    port = args.port or int(os.environ.get(PORT_ENV) or DEFAULT_PORT)
    try:
        with VtControl(port, args.host) as vt:
            result = run(vt, args)
    except VtUnavailable as exc:
        print(f"error: {exc}", file=sys.stderr)
        return 2
    except VtRefused as exc:
        print(f"refused: {exc}", file=sys.stderr)
        return 1
    if args.command == "screen" and not args.json:
        print("\n".join(screen_lines(result)))
    else:
        print(json.dumps(result, indent=2))
    if args.command in ("wait-mask", "wait-text"):
        return 0 if result["met"] else 1
    return 0


if __name__ == "__main__":
    sys.exit(main())
