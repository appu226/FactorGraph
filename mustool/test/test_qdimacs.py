from qdimacs import Qdimacs
import unittest
import testUtils

class TestQdimacsParsing(unittest.TestCase):

  def test_simple_parsing(self):
    import os
    filepath = os.path.join(testUtils.getTestPath(), 'parsingTest.qdimacs')
    q = Qdimacs(filepath)
    self.assertEqual(q.numVars, 10)
    self.assertEqual(q.numClauses, 3)
    self.assertEqual(q.quantifiers, [['a', 3, 4], ['e', 1, 2, 5], ['a', 6]])
    self.assertEqual(q.clauses,[[1, 2, 3, -4, 5], [2, 7, 8, -3, 10], [-7, -8, 1, 2, -6, 9]])
    


if __name__ == '__main__':
  unittest.main()
    
