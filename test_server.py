import json
import unittest
import uuid
from unittest.mock import patch
import server


def packet():
    return dict(schema=1, capture_id=str(uuid.uuid4()), source_pid=1234,
                version=2000 << 16, items=[dict(id=3, name='한글.txt', size=8)])


class Tests(unittest.TestCase):
    def setUp(self):
        server.snapshot = None

    def test_roundtrip(self):
        obj = packet(); server.accept(json.dumps(obj).encode())
        result = server.selected_items()
        self.assertEqual(result['items'], obj['items'])
        self.assertFalse(result['live'])

    def test_invalid(self):
        for change in ({'schema': True}, {'source_pid': -1}, {'items': [dict(id=1,name='x',size=True)]},
                       {'capture_id': 'bad'}, {'capture_id': []},
                       {'items': packet()['items'] * 2}, {'command': 'shell'}):
            with self.subTest(change=change), self.assertRaises((ValueError, TypeError)):
                server.validate(json.dumps(packet() | change).encode())
        with self.assertRaises(ValueError): server.validate(b' ' * (server.MAX_BYTES + 1))

    def test_expiry_and_capture(self):
        self.assertFalse(server.selected_items()['available'])
        server.accept(json.dumps(packet()).encode())
        with self.assertRaises(ValueError): server.selected_items(capture_id='different')
        with patch('server.time.monotonic', return_value=server.received + 301):
            self.assertFalse(server.selected_items()['available'])
        with self.assertRaises(ValueError): server.selected_items(limit=101)


if __name__ == '__main__': unittest.main()
