def all(iterable):
    for i in iterable:
        if not i:
            return False
    return True

def any(iterable):
    for i in iterable:
        if i:
            return True
    return False

def enumerate(iterable, start=0):
    n = start
    for elem in iterable:
        yield n, elem
        n += 1

def __minmax_reduce(op, args):
    if len(args) == 2:  # min(1, 2)
        return args[0] if op(args[0], args[1]) else args[1]
    if len(args) == 0:  # min()
        raise TypeError('expected 1 arguments, got 0')
    if len(args) == 1:  # min([1, 2, 3, 4]) -> min(1, 2, 3, 4)
        args = args[0]
    args = iter(args)
    try:
        res = next(args)
    except StopIteration:
        raise ValueError('args is an empty sequence')
    while True:
        try:
            i = next(args)
        except StopIteration:
            break
        if op(i, res):
            res = i
    return res

def min(*args, key=None):
    key = key or (lambda x: x)
    return __minmax_reduce(lambda x,y: key(x)<key(y), args)

def max(*args, key=None):
    key = key or (lambda x: x)
    return __minmax_reduce(lambda x,y: key(x)>key(y), args)

def sum(iterable):
    res = 0
    for i in iterable:
        res += i
    return res

def map(f, iterable):
    for i in iterable:
        yield f(i)

def filter(f, iterable):
    for i in iterable:
        if f(i):
            yield i

def zip(a, b):
    a = iter(a)
    b = iter(b)
    while True:
        try:
            ai = next(a)
            bi = next(b)
        except StopIteration:
            break
        yield ai, bi

def reversed(iterable):
    a = list(iterable)
    a.reverse()
    return a

def sorted(iterable, key=None, reverse=False):
    a = list(iterable)
    a.sort(key=key, reverse=reverse)
    return a

##### str #####
def __format_string(self: str, *args, **kwargs) -> str:
    def tokenizeString(s: str):
        tokens = []
        L, R = 0,0
        
        mode = None
        curArg = 0
        # lookingForKword = False
        
        while(R<len(s)):
            curChar = s[R]
            nextChar = s[R+1] if R+1<len(s) else ''
            
            # Invalid case 1: stray '}' encountered, example: "ABCD EFGH {name} IJKL}", "Hello {vv}}", "HELLO {0} WORLD}"
            if curChar == '}' and nextChar != '}':
                raise ValueError("Single '}' encountered in format string")        
            
            # Valid Case 1: Escaping case, we escape "{{ or "}}" to be "{" or "}", example: "{{}}", "{{My Name is {0}}}"
            if (curChar == '{' and nextChar == '{') or (curChar == '}' and nextChar == '}'):
                
                if (L<R): # Valid Case 1.1: make sure we are not adding empty string
                    tokens.append(s[L:R]) # add the string before the escape
                
                
                tokens.append(curChar) # Valid Case 1.2: add the escape char
                L = R+2 # move the left pointer to the next char
                R = R+2 # move the right pointer to the next char
                continue
            
            # Valid Case 2: Regular command line arg case: example:  "ABCD EFGH {} IJKL", "{}", "HELLO {} WORLD"
            elif curChar == '{' and nextChar == '}':
                if mode is not None and mode != 'auto':
                    # Invalid case 2: mixing automatic and manual field specifications -- example: "ABCD EFGH {name} IJKL {}", "Hello {vv} {}", "HELLO {0} WORLD {}" 
                    raise ValueError("Cannot switch from manual field numbering to automatic field specification")
                
                mode = 'auto'
                if(L<R): # Valid Case 2.1: make sure we are not adding empty string
                    tokens.append(s[L:R]) # add the string before the special marker for the arg
                
                tokens.append("{"+str(curArg)+"}") # Valid Case 2.2: add the special marker for the arg
                curArg+=1 # increment the arg position, this will be used for referencing the arg later
                
                L = R+2 # move the left pointer to the next char
                R = R+2 # move the right pointer to the next char
                continue
            
            # Valid Case 3: Key-word arg case: example: "ABCD EFGH {name} IJKL", "Hello {vv}", "HELLO {name} WORLD"
            elif (curChar == '{'):
                
                if mode is not None and mode != 'manual':
                    # # Invalid case 2: mixing automatic and manual field specifications -- example: "ABCD EFGH {} IJKL {name}", "Hello {} {1}", "HELLO {} WORLD {name}"
                    raise ValueError("Cannot switch from automatic field specification to manual field numbering")
                
                mode = 'manual'
                
                if(L<R): # Valid case 3.1: make sure we are not adding empty string
                    tokens.append(s[L:R]) # add the string before the special marker for the arg
                
                # We look for the end of the keyword          
                kwL = R # Keyword left pointer
                kwR = R+1 # Keyword right pointer
                while(kwR<len(s) and s[kwR]!='}'):
                    if s[kwR] == '{': # Invalid case 3: stray '{' encountered, example: "ABCD EFGH {n{ame} IJKL {", "Hello {vv{}}", "HELLO {0} WOR{LD}"
                        raise ValueError("Unexpected '{' in field name")
                    kwR += 1
                
                # Valid case 3.2: We have successfully found the end of the keyword
                if kwR<len(s) and s[kwR] == '}':
                    tokens.append(s[kwL:kwR+1]) # add the special marker for the arg
                    L = kwR+1
                    R = kwR+1
                    
                # Invalid case 4: We didn't find the end of the keyword, throw error
                else:
                    raise ValueError("Expected '}' before end of string")
                continue
            
            R = R+1
        
        
        # Valid case 4: We have reached the end of the string, add the remaining string to the tokens 
        if L<R:
            tokens.append(s[L:R])
                
        # print(tokens)
        return tokens

    tokens = tokenizeString(self)
    argMap = {}
    for i, a in enumerate(args):
        argMap[str(i)] = a
    final_tokens = []
    for t in tokens:
        if t[0] == '{' and t[-1] == '}':
            key = t[1:-1]
            argMapVal = argMap.get(key, None)
            kwargsVal = kwargs.get(key, None)
                                    
            if argMapVal is None and kwargsVal is None:
                raise ValueError("No arg found for token: "+t)
            elif argMapVal is not None:
                final_tokens.append(str(argMapVal))
            else:
                final_tokens.append(str(kwargsVal))
        else:
            final_tokens.append(t)
    
    return ''.join(final_tokens)

str.format = __format_string
del __format_string


def help(obj):
    if hasattr(obj, '__func__'):
        obj = obj.__func__
    # print(obj.__signature__)
    if obj.__doc__:
        print(obj.__doc__)

def complex(real, imag=0):
    import cmath
    return cmath.complex(real, imag) # type: ignore

def dir(obj) -> list[str]:
    tp_module = type(__import__('math'))
    if isinstance(obj, tp_module):
        return [k for k, _ in obj.__dict__.items()]
    names = set()
    if not isinstance(obj, type):
        obj_d = obj.__dict__
        if obj_d is not None:
            names.update([k for k, _ in obj_d.items()])
        cls = type(obj)
    else:
        cls = obj
    while cls is not None:
        names.update([k for k, _ in cls.__dict__.items()])
        cls = cls.__base__
    return sorted(list(names))

class set:
    def __init__(self, iterable=None):
        iterable = iterable or []
        self._a = {}
        self.update(iterable)

    def add(self, elem):
        self._a[elem] = None
        
    def discard(self, elem):
        self._a.pop(elem, None)

    def remove(self, elem):
        del self._a[elem]
        
    def clear(self):
        self._a.clear()

    def update(self, other):
        for elem in other:
            self.add(elem)

    def __len__(self):
        return len(self._a)
    
    def copy(self):
        return set(self._a.keys())
    
    def __and__(self, other):
        return {elem for elem in self if elem in other}

    def __sub__(self, other):
        return {elem for elem in self if elem not in other}
    
    def __or__(self, other):
        ret = self.copy()
        ret.update(other)
        return ret

    def __xor__(self, other): 
        _0 = self - other
        _1 = other - self
        return _0 | _1

    def union(self, other):
        return self | other

    def intersection(self, other):
        return self & other

    def difference(self, other):
        return self - other

    def symmetric_difference(self, other):      
        return self ^ other
    
    def __eq__(self, other):
        if not isinstance(other, set):
            return NotImplemented
        return len(self ^ other) == 0
    
    def __ne__(self, other):
        if not isinstance(other, set):
            return NotImplemented
        return len(self ^ other) != 0

    def isdisjoint(self, other):
        return len(self & other) == 0
    
    def issubset(self, other):
        return len(self - other) == 0
    
    def issuperset(self, other):
        return len(other - self) == 0

    def __contains__(self, elem):
        return elem in self._a
    
    def __repr__(self):
        if len(self) == 0:
            return 'set()'
        return '{'+ ', '.join([repr(i) for i in self._a.keys()]) + '}'
    
    def __iter__(self):
        return iter(self._a.keys())