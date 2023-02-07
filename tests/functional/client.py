from enum import IntEnum
from ragger.backend import BackendInterface
from ragger.bip.path import pack_derivation_path
from ragger.firmware import Firmware
from ragger.firmware.stax import MetaScreen
from ragger.firmware.stax.use_cases import UseCaseChoice
from ragger.navigator import NavInsID, NanoNavigator
from typing import List, Optional


def change_path_endianness(path: bytes) -> bytes:
    # expects a LV bip32 path: <length> <4B> <4B> <4B> ...
    assert (path[0] * 4 + 1) == len(path)
    result = path[0:1]
    for i in range(path[0]):
        segment = path[1 + i * 4 : 1 + (i + 1) * 4]
        result += bytes(reversed(segment))
    return result


def bip32(path: str) -> bytes:
    return change_path_endianness(pack_derivation_path(path))


CLA = 0x55
HRP = bytearray.fromhex("696f")

class Instruction(IntEnum):
    GET_VERSION               = 0x00
    PUBLIC_KEY_SECP256K1      = 0x01  # Will be deprecated in the near future
    SIGN_SECP256K1            = 0x02
    # SHOW_ADDR_SECP256K1       = 0x03  # Deprecated
    GET_ADDR_SECP256K1        = 0x04
    SIGN_PERSONAL_MESSAGE     = 0x05
    HASH_TEST                 = 0x64
    PUBLIC_KEY_SECP256K1_TEST = 0x65
    SIGN_SECP256K1_TEST       = 0x66


class IotexClient(metaclass=MetaScreen):
    use_case_choice = UseCaseChoice

    def __init__(self, backend: BackendInterface, firmware: Firmware, derivation_path: Optional[str] = None):
        self._backend = backend
        self._firmware = firmware
        self._derivation_path = bip32(derivation_path) if derivation_path else None
        self._navigator = NanoNavigator(backend, firmware)

    @property
    def derivation_path(self) -> bytes:
        assert self._derivation_path is not None, "A derivation path must be set"
        return self._derivation_path

    @derivation_path.setter
    def derivation_path(self, value: str) -> None:
        self._derivation_path = bip32(value)

    def get_public_key(self) -> bytes:
        payload = self.derivation_path
        return self._backend.exchange(CLA, Instruction.PUBLIC_KEY_SECP256K1, data=payload).data

    def _approve_get_address(self) -> None:
        if self._firmware.device == "nanos":
            self._backend.right_click()
        elif self._firmware.device in ["nanox", "nanosp"]:
            self._backend.right_click()  # display address
            self._backend.right_click()  # display validation screen
            self._backend.both_click()  # validate
        elif self._firmware.device == "stax":
            self.choice.confirm()

    def get_address(self) -> bytes:
        payload = bytes([len(HRP)]) + HRP + self.derivation_path
        with self._backend.exchange_async(CLA, Instruction.GET_ADDR_SECP256K1, data=payload):
            self._approve_get_address()
        return self._backend.last_async_response.data

    def _init_signature(self) -> bytes:
        payload = self.derivation_path
        return self._backend.exchange(CLA,
                                      Instruction.SIGN_PERSONAL_MESSAGE,
                                      p1=0x01,
                                      p2=0x02,
                                      data=payload).data

    def _sign(self, ins: Instruction, payload: bytes, navigation: List[NavInsID]) -> bytes:
        self._init_signature()
        with self._backend.exchange_async(CLA, ins, p1=0x02, p2=0x02, data=payload):
            self._navigator.navigate(navigation)
        return self._backend.last_async_response.data

    def sign(self, payload: bytes, navigation: List[NavInsID]) -> bytes:
        return self._sign(Instruction.SIGN_PERSONAL_MESSAGE, payload, navigation)

    def sign_raw(self, payload: bytes, navigation: List[NavInsID]) -> bytes:
        return self._sign(Instruction.SIGN_PERSONAL_MESSAGE, payload, navigation)
