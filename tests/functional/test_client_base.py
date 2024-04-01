from client import IotexClient
from dataset import DataTestCase


SPECULOS_PUBLIC_KEY = bytes.fromhex("0435c29a7915347e0d44c39f277e0668d59eb7180e99954f67285ea8eae62a5bd"
                                    "ad625a01be2705ed07572541854350ebcfc4c880eaa37121b9b6d2613cf7eb31b")
SPECULOS_ADDRESS = bytes.fromhex("0335c29a7915347e0d44c39f277e0668d59eb7180e99954f67285ea8eae62a5bda696f3173"
                                 "75306c656c6878747437646b6e616d3967673863687661706663677379687065636478646d")


def test_get_public_key(client: IotexClient):
    assert client.get_public_key() == SPECULOS_PUBLIC_KEY

def test_get_address(client: IotexClient):
    assert client.get_address() == SPECULOS_ADDRESS

def test_signature(client: IotexClient, test_case: DataTestCase):
    assert client.sign(test_case.input, test_case.validation) == test_case.output
