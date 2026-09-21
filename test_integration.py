"""Synthetic data only: real SDK stdio handshake + real Windows Named Pipe."""
import asyncio
import json
import sys
import unittest
from pathlib import Path
import win32con
import win32file
from mcp import ClientSession, StdioServerParameters
from mcp.client.stdio import stdio_client
from server import pipe_settings
from test_server import packet


class Integration(unittest.IsolatedAsyncioTestCase):
    async def test_stdio_pipe(self):
        args = StdioServerParameters(command=sys.executable, args=[str(Path(__file__).with_name('server.py'))])
        async with stdio_client(args) as (read, write):
            async with ClientSession(read, write) as client:
                await client.initialize()
                listed = await client.list_tools()
                self.assertEqual({t.name for t in listed.tools},
                                 {'bridge_status', 'selected_items', 'problem_definition'})
                for _ in range(100):
                    status = await client.call_tool('bridge_status', {})
                    if json.loads(status.content[0].text)['listener_ready']: break
                    await asyncio.sleep(.05)
                else: self.fail('Pipe listener did not start')
                name, _ = pipe_settings()
                payload = packet()
                handle = win32file.CreateFile(name, win32con.GENERIC_READ | win32con.GENERIC_WRITE, 0, None,
                                             win32con.OPEN_EXISTING, 0, None)
                try:
                    win32file.WriteFile(handle, json.dumps(payload).encode())
                    code, ack = await asyncio.wait_for(asyncio.to_thread(win32file.ReadFile, handle, 1), 5)
                    self.assertEqual((code, ack), (0, b'1'))
                finally: handle.Close()
                for _ in range(100):
                    result = await client.call_tool('selected_items', {})
                    value = json.loads(result.content[0].text)
                    if value.get('available'): break
                    await asyncio.sleep(.05)
                else: self.fail('Snapshot was not received')
                self.assertEqual(value['items'], payload['items'])
                self.assertFalse(value['live'])


if __name__ == '__main__': unittest.main()
