from dataclasses import dataclass
from typing import List


@dataclass
class TestCase:
    name: str
    _input: str
    _output: str

    @property
    def input(self) -> bytes:
        return bytes.fromhex(self._input)

    @property
    def output(self) -> bytes:
        return bytes.fromhex(self._output)


TEST_CASES = [
    TestCase("hello", "68656c6c6f", "1c6d22cbd8f3025fb66e18cc61cd804f1ed88bc4e5769032d4f9e51df8e1cb4d6e41a537a7574a3bc111bb09c51d4cc6ff10e4094dd2d5847980e169519838aa00"),

]
