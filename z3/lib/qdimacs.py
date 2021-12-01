import z3

class Qdimacs:
  def __init__(self, filename):
    self.quantifiers = []
    self.clauses = []
    self.numVars = None
    self.numClauses = None
    with open(filename) as f:
      for line in f:
        if len(line.strip()) == 0:
          continue
        tokens = line.split(' ')
        if len(tokens) == 0 or tokens[0] == 'c':
          pass
        elif tokens[0] == 'p' and len(tokens) == 4 and tokens[1] == 'cnf':
          self.numVars = int(tokens[2])
          self.numClauses = int(tokens[3])
        elif tokens[0] in ['a', 'e']:
          assert(tokens[-1].strip() == '0')
          qclause = tokens[0:-1]
          for i in range(1, len(qclause)):
            qclause[i] = int(qclause[i])
          self.quantifiers.append(qclause)
          for variable in self.quantifiers[-1][1:]:
            assert(variable > 0 and variable <= self.numVars)
        else:
          assert(tokens[-1].strip() == '0')
          self.clauses.append(list(map(int, tokens[0:-1])))
          for variable in self.clauses[-1]:
            assert(abs(variable) > 0 and abs(variable) <= self.numVars)
    assert(len(self.clauses) == self.numClauses)






