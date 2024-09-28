import unittest

import basic # type: ignore


class TestBasic(unittest.TestCase):

  def test_add(self):
    pet = basic.Pet("dog")
    self.assertEqual(pet.name, "dog")


if __name__ == "__main__":
  unittest.main()
