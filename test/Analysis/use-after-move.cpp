// RUN: %clang_analyze_cc1 -analyzer-checker=alpha.cplusplus.Move -verify %s\
// RUN:  -std=c++11 -analyzer-output=text -analyzer-config eagerly-assume=false\
// RUN:  -analyzer-config exploration_strategy=unexplored_first_queue
// RUN: %clang_analyze_cc1 -analyzer-checker=alpha.cplusplus.Move -verify %s\
// RUN:  -std=c++11 -analyzer-output=text -analyzer-config eagerly-assume=false\
// RUN:  -analyzer-config exploration_strategy=dfs -DDFS=1
// RUN: %clang_analyze_cc1 -analyzer-checker=alpha.cplusplus.Move -verify %s\
// RUN:  -std=c++11 -analyzer-output=text -analyzer-config eagerly-assume=false\
// RUN:  -analyzer-config exploration_strategy=unexplored_first_queue\
// RUN:  -analyzer-config alpha.cplusplus.Move:Aggressive=true -DAGGRESSIVE
// RUN: %clang_analyze_cc1 -analyzer-checker=alpha.cplusplus.Move -verify %s\
// RUN:  -std=c++11 -analyzer-output=text -analyzer-config eagerly-assume=false\
// RUN:  -analyzer-config exploration_strategy=dfs -DDFS=1\
// RUN:  -analyzer-config alpha.cplusplus.Move:Aggressive=true -DAGGRESSIVE

namespace std {

typedef __typeof(sizeof(int)) size_t;

template <typename>
struct remove_reference;

template <typename _Tp>
struct remove_reference { typedef _Tp type; };

template <typename _Tp>
struct remove_reference<_Tp &> { typedef _Tp type; };

template <typename _Tp>
struct remove_reference<_Tp &&> { typedef _Tp type; };

template <typename _Tp>
typename remove_reference<_Tp>::type &&move(_Tp &&__t) {
  return static_cast<typename remove_reference<_Tp>::type &&>(__t);
}

template <typename _Tp>
_Tp &&forward(typename remove_reference<_Tp>::type &__t) noexcept {
  return static_cast<_Tp &&>(__t);
}

template <class T>
void swap(T &a, T &b) {
  T c(std::move(a));
  a = std::move(b);
  b = std::move(c);
}

template <typename T>
class vector {
public:
  vector();
  void push_back(const T &t);
};

} // namespace std

class B {
public:
  B() = default;
  B(const B &) = default;
  B(B &&) = default;
  B& operator=(const B &q) = default;
  void operator=(B &&b) {
    return;
  }
  void foo() { return; }
};

class A {
  int i;
  double d;

public:
  B b;
  A(int ii = 42, double dd = 1.0) : d(dd), i(ii), b(B()) {}
  void moveconstruct(A &&other) {
    std::swap(b, other.b);
    std::swap(d, other.d);
    std::swap(i, other.i);
    return;
  }
  static A get() {
    A v(12, 13);
    return v;
  }
  A(A *a) {
    moveconstruct(std::move(*a));
  }
  A(const A &other) : i(other.i), d(other.d), b(other.b) {}
  A(A &&other) : i(other.i), d(other.d), b(std::move(other.b)) {
#ifdef AGGRESSIVE
    // expected-note@-2{{Object 'b' is moved}}
#endif
  }
  A(A &&other, char *k) {
    moveconstruct(std::move(other));
  }
  void operator=(const A &other) {
    i = other.i;
    d = other.d;
    b = other.b;
    return;
  }
  void operator=(A &&other) {
    moveconstruct(std::move(other));
    return;
  }
  int getI() { return i; }
  int foo() const;
  void bar() const;
  void reset();
  void destroy();
  void clear();
  void resize(std::size_t);
  bool empty() const;
  bool isEmpty() const;
  operator bool() const;
};

int bignum();

void moveInsideFunctionCall(A a) {
  A b = std::move(a);
}
void leftRefCall(A &a) {
  a.foo();
}
void rightRefCall(A &&a) {
  a.foo();
}
void constCopyOrMoveCall(const A a) {
  a.foo();
}

void copyOrMoveCall(A a) {
  a.foo();
}

void simpleMoveCtorTest() {
  {
    A a;
    A b = std::move(a); // expected-note {{Object 'a' is moved}}
    a.foo();            // expected-warning {{Method called on moved-from object 'a'}} expected-note {{Method called on moved-from object 'a'}}
  }
  {
    A a;
    A b = std::move(a); // expected-note {{Object 'a' is moved}}
    b = a;              // expected-warning {{Moved-from object 'a' is copied}} expected-note {{Moved-from object 'a' is copied}}
  }
  {
    A a;
    A b = std::move(a); // expected-note {{Object 'a' is moved}}
    b = std::move(a);   // expected-warning {{Moved-from object 'a' is moved}} expected-note {{Moved-from object 'a' is moved}}
  }
}

void simpleMoveAssignementTest() {
  {
    A a;
    A b;
    b = std::move(a); // expected-note {{Object 'a' is moved}}
    a.foo();          // expected-warning {{Method called on moved-from object 'a'}} expected-note {{Method called on moved-from object 'a'}}
  }
  {
    A a;
    A b;
    b = std::move(a); // expected-note {{Object 'a' is moved}}
    A c(a);           // expected-warning {{Moved-from object 'a' is copied}} expected-note {{Moved-from object 'a' is copied}}
  }
  {
    A a;
    A b;
    b = std::move(a);  // expected-note {{Object 'a' is moved}}
    A c(std::move(a)); // expected-warning {{Moved-from object 'a' is moved}} expected-note {{Moved-from object 'a' is moved}}
  }
}

void moveInInitListTest() {
  struct S {
    A a;
  };
  A a;
  S s{std::move(a)}; // expected-note {{Object 'a' is moved}}
  a.foo();           // expected-warning {{Method called on moved-from object 'a'}} expected-note {{Method called on moved-from object 'a'}}
}

// Don't report a bug if the variable was assigned to in the meantime.
void reinitializationTest(int i) {
  {
    A a;
    A b;
    b = std::move(a);
    a = A();
    a.foo();
  }
  {
    A a;
    if (i == 1) { // expected-note {{Assuming 'i' is not equal to 1}} expected-note {{Taking false branch}}
      // expected-note@-1 {{Assuming 'i' is not equal to 1}} expected-note@-1 {{Taking false branch}}
      A b;
      b = std::move(a);
      a = A();
    }
    if (i == 2) { // expected-note {{Assuming 'i' is not equal to 2}} expected-note {{Taking false branch}}
      //expected-note@-1 {{Assuming 'i' is not equal to 2}} expected-note@-1 {{Taking false branch}}
      a.foo();    // no-warning
    }
  }
  {
    A a;
    if (i == 1) { // expected-note {{Taking false branch}} expected-note {{Taking false branch}}
      std::move(a);
    }
    if (i == 2) { // expected-note {{Taking false branch}} expected-note {{Taking false branch}}
      a = A();
      a.foo();
    }
  }
  // The built-in assignment operator should also be recognized as a
  // reinitialization. (std::move() may be called on built-in types in template
  // code.)
  {
    int a1 = 1, a2 = 2;
    std::swap(a1, a2);
  }
  // A std::move() after the assignment makes the variable invalid again.
  {
    A a;
    A b;
    b = std::move(a);
    a = A();
    b = std::move(a); // expected-note {{Object 'a' is moved}}
    a.foo();          // expected-warning {{Method called on moved-from object 'a'}} expected-note {{Method called on moved-from object 'a'}}
  }
  // If a path exist where we not reinitialize the variable we report a bug.
  {
    A a;
    A b;
    b = std::move(a); // expected-note {{Object 'a' is moved}}
    if (i < 10) {     // expected-note {{Assuming 'i' is >= 10}} expected-note {{Taking false branch}}
      a = A();
    }
    if (i > 5) { // expected-note {{Taking true branch}}
      a.foo();   // expected-warning {{Method called on moved-from object 'a'}} expected-note {{Method called on moved-from object 'a'}}
    }
  }
}

// Using decltype on an expression is not a use.
void decltypeIsNotUseTest() {
  A a;
  // A b(std::move(a));
  decltype(a) other_a; // no-warning
}

void loopTest() {
  {
    A a;
    for (int i = 0; i < bignum(); i++) { // expected-note {{Loop condition is false. Execution jumps to the end of the function}}
      rightRefCall(std::move(a));        // no-warning
    }
  }
  {
    A a;
    for (int i = 0; i < 2; i++) { // expected-note {{Loop condition is true.  Entering loop body}}
      //expected-note@-1 {{Loop condition is true.  Entering loop body}}
			//expected-note@-2 {{Loop condition is false. Execution jumps to the end of the function}}
      rightRefCall(std::move(a)); // no-warning
    }
  }
  {
    A a;
    for (int i = 0; i < bignum(); i++) { // expected-note {{Loop condition is false. Execution jumps to the end of the function}}
      leftRefCall(a);                    // no-warning
    }
  }
  {
    A a;
    for (int i = 0; i < 2; i++) { // expected-note {{Loop condition is true.  Entering loop body}} 
      //expected-note@-1 {{Loop condition is true.  Entering loop body}}
			//expected-note@-2 {{Loop condition is false. Execution jumps to the end of the function}}
      leftRefCall(a);             // no-warning
    }
  }
  {
    A a;
    for (int i = 0; i < bignum(); i++) { // expected-note {{Loop condition is false. Execution jumps to the end of the function}}
      constCopyOrMoveCall(a);            // no-warning
    }
  }
  {
    A a;
    for (int i = 0; i < 2; i++) { // expected-note {{Loop condition is true.  Entering loop body}} 
      //expected-note@-1 {{Loop condition is true.  Entering loop body}}
			//expected-note@-2 {{Loop condition is false. Execution jumps to the end of the function}}
      constCopyOrMoveCall(a);     // no-warning
    }
  }
  {
    A a;
    for (int i = 0; i < bignum(); i++) { // expected-note {{Loop condition is false. Execution jumps to the end of the function}}
      moveInsideFunctionCall(a);         // no-warning
    }
  }
  {
    A a;
    for (int i = 0; i < 2; i++) { // expected-note {{Loop condition is true.  Entering loop body}}
      //expected-note@-1 {{Loop condition is true.  Entering loop body}}
			//expected-note@-2 {{Loop condition is false. Execution jumps to the end of the function}}
      moveInsideFunctionCall(a);  // no-warning
    }
  }
  {
    A a;
    for (int i = 0; i < bignum(); i++) { // expected-note {{Loop condition is false. Execution jumps to the end of the function}}
      copyOrMoveCall(a);                 // no-warning
    }
  }
  {
    A a;
    for (int i = 0; i < 2; i++) { // expected-note {{Loop condition is true.}}
      //expected-note@-1 {{Loop condition is true.  Entering loop body}}
			//expected-note@-2 {{Loop condition is false. Execution jumps to the end of the function}}
      copyOrMoveCall(a);          // no-warning
    }
  }
  {
    A a;
    for (int i = 0; i < bignum(); i++) { // expected-note {{Loop condition is true.  Entering loop body}} expected-note {{Loop condition is true.  Entering loop body}}
      constCopyOrMoveCall(std::move(a)); // expected-warning {{Moved-from object 'a' is moved}} expected-note {{Moved-from object 'a' is moved}}
      // expected-note@-1 {{Object 'a' is moved}}
    }
  }

  // Don't warn if we return after the move.
  {
    A a;
    for (int i = 0; i < 3; ++i) {
      a.bar();
      if (a.foo() > 0) {
        A b;
        b = std::move(a); // no-warning
        return;
      }
    }
  }
}

//report a usage of a moved-from object only at the first use
void uniqueTest(bool cond) {
  A a(42, 42.0);
  A b;
  b = std::move(a); // expected-note {{Object 'a' is moved}}

  if (cond) { // expected-note {{Assuming 'cond' is not equal to 0}} expected-note {{Taking true branch}}
    a.foo();  // expected-warning {{Method called on moved-from object 'a'}} expected-note {{Method called on moved-from object 'a'}}
  }
  if (cond) {
    a.bar(); // no-warning
  }

  a.bar(); // no-warning
}

void uniqueTest2() {
  A a;
  A a1 = std::move(a); // expected-note {{Object 'a' is moved}}
  a.foo();             // expected-warning {{Method called on moved-from object 'a'}} expected-note {{Method called on moved-from object 'a'}}

  A a2 = std::move(a); // no-warning
  a.foo();             // no-warning
}

// There are exceptions where we assume in general that the method works fine
//even on moved-from objects.
void moveSafeFunctionsTest() {
  A a;
  A b = std::move(a); // expected-note {{Object 'a' is moved}}
  a.empty();          // no-warning
  a.isEmpty();        // no-warning
  (void)a;            // no-warning
  (bool)a;            // expected-warning {{expression result unused}}
  a.foo();            // expected-warning {{Method called on moved-from object 'a'}} expected-note {{Method called on moved-from object 'a'}}
}

void moveStateResetFunctionsTest() {
  {
    A a;
    A b = std::move(a);
    a.reset(); // no-warning
    a.foo();   // no-warning
    // Test if resets the state of subregions as well.
    a.b.foo(); // no-warning
  }
  {
    A a;
    A b = std::move(a);
    a.destroy(); // no-warning
    a.foo();     // no-warning
  }
  {
    A a;
    A b = std::move(a);
    a.clear(); // no-warning
    a.foo();   // no-warning
    a.b.foo(); // no-warning
  }
  {
    A a;
    A b = std::move(a);
    a.resize(0); // no-warning
    a.foo();   // no-warning
    a.b.foo(); // no-warning
  }
}

// Moves or uses that occur as part of template arguments.
template <int>
class ClassTemplate {
public:
  void foo(A a);
};

template <int>
void functionTemplate(A a);

void templateArgIsNotUseTest() {
  {
    // A pattern like this occurs in the EXPECT_EQ and ASSERT_EQ macros in
    // Google Test.
    A a;
    ClassTemplate<sizeof(A(std::move(a)))>().foo(std::move(a)); // no-warning
  }
  {
    A a;
    functionTemplate<sizeof(A(std::move(a)))>(std::move(a)); // no-warning
  }
}

// Moves of global variables are not reported.
A global_a;
void globalVariablesTest() {
  std::move(global_a);
  global_a.foo(); // no-warning
}

// Moves of member variables.
class memberVariablesTest {
  A a;
  static A static_a;

  void f() {
    A b;
    b = std::move(a);
    a.foo();
#ifdef AGGRESSIVE
    // expected-note@-3{{Object 'a' is moved}}
    // expected-warning@-3 {{Method called on moved-from object 'a'}}
    // expected-note@-4{{Method called on moved-from object 'a'}}
#endif

    b = std::move(static_a);
    static_a.foo();
#ifdef AGGRESSIVE
    // expected-note@-3{{Object 'static_a' is moved}}
    // expected-warning@-3{{Method called on moved-from object 'static_a'}}
    // expected-note@-4{{Method called on moved-from object 'static_a'}}
#endif
  }
};

void PtrAndArrayTest() {
  A *Ptr = new A(1, 1.5);
  A Arr[10];
  Arr[2] = std::move(*Ptr);
  (*Ptr).foo();
#ifdef AGGRESSIVE
  // expected-note@-3{{Object is moved}}
  // expected-warning@-3{{Method called on moved-from object}}
  // expected-note@-4{{Method called on moved-from object}}
#endif

  Ptr = &Arr[1];
  Arr[3] = std::move(Arr[1]);
  Ptr->foo();
#ifdef AGGRESSIVE
  // expected-note@-3{{Object is moved}}
  // expected-warning@-3{{Method called on moved-from object}}
  // expected-note@-4{{Method called on moved-from object}}
#endif

  Arr[3] = std::move(Arr[2]);
  Arr[2].foo();
#ifdef AGGRESSIVE
  // expected-note@-3{{Object is moved}}
  // expected-warning@-3{{Method called on moved-from object}}
  // expected-note@-4{{Method called on moved-from object}}
#endif

  Arr[2] = std::move(Arr[3]); // reinitialization
  Arr[2].foo();               // no-warning
}

void exclusiveConditionsTest(bool cond) {
  A a;
  if (cond) {
    A b;
    b = std::move(a);
  }
  if (!cond) {
    a.bar(); // no-warning
  }
}

void differentBranchesTest(int i) {
  // Don't warn if the use is in a different branch from the move.
  {
    A a;
    if (i > 0) { // expected-note {{Assuming 'i' is > 0}} expected-note {{Taking true branch}}
      A b;
      b = std::move(a);
    } else {
      a.foo(); // no-warning
    }
  }
  // Same thing, but with a ternary operator.
  {
    A a, b;
    i > 0 ? (void)(b = std::move(a)) : a.bar(); // no-warning  // expected-note {{'?' condition is true}}
  }
  // A variation on the theme above.
  {
    A a;
#ifdef DFS
    a.foo() > 0 ? a.foo() : A(std::move(a)).foo(); // expected-note {{Assuming the condition is false}} expected-note {{'?' condition is false}}
#else
    a.foo() > 0 ? a.foo() : A(std::move(a)).foo(); // expected-note {{Assuming the condition is true}} expected-note {{'?' condition is true}}
#endif
  }
  // Same thing, but with a switch statement.
  {
    A a, b;
    switch (i) { // expected-note {{Control jumps to 'case 1:'}}
    case 1:
      b = std::move(a); // no-warning
      break;            // expected-note {{Execution jumps to the end of the function}}
    case 2:
      a.foo(); // no-warning
      break;
    }
  }
  // However, if there's a fallthrough, we do warn.
  {
    A a, b;
    switch (i) { // expected-note {{Control jumps to 'case 1:'}}
    case 1:
      b = std::move(a); // expected-note {{Object 'a' is moved}}
    case 2:
      a.foo(); // expected-warning {{Method called on moved-from object}} expected-note {{Method called on moved-from object 'a'}}
      break;
    }
  }
}

void tempTest() {
  A a = A::get();
  A::get().foo(); // no-warning
  for (int i = 0; i < bignum(); i++) {
    A::get().foo(); // no-warning
  }
}

void interFunTest1(A &a) {
  a.bar(); // expected-warning {{Method called on moved-from object 'a'}} expected-note {{Method called on moved-from object 'a'}}
}

void interFunTest2() {
  A a;
  A b;
  b = std::move(a); // expected-note {{Object 'a' is moved}}
  interFunTest1(a); // expected-note {{Calling 'interFunTest1'}}
}

void foobar(A a, int i);
void foobar(int i, A a);

void paramEvaluateOrderTest() {
  A a;
  foobar(std::move(a), a.getI()); // expected-warning {{Method called on moved-from object 'a'}} expected-note {{Method called on moved-from object 'a'}}
  // expected-note@-1 {{Object 'a' is moved}}

  //FALSE NEGATIVE since parameters evaluate order is undefined
  foobar(a.getI(), std::move(a)); //no-warning
}

void not_known(A &a);
void not_known(A *a);

void regionAndPointerEscapeTest() {
  {
    A a;
    A b;
    b = std::move(a);
    not_known(a);
    a.foo(); //no-warning
  }
  {
    A a;
    A b;
    b = std::move(a);
    not_known(&a);
    a.foo(); // no-warning
  }
}

// A declaration statement containing multiple declarations sequences the
// initializer expressions.
void declarationSequenceTest() {
  {
    A a;
    A a1 = a, a2 = std::move(a); // no-warning
  }
  {
    A a;
    A a1 = std::move(a), a2 = a; // expected-warning {{Moved-from object 'a' is copied}} expected-note {{Moved-from object 'a' is copied}}
    // expected-note@-1 {{Object 'a' is moved}}
  }
}

// The logical operators && and || sequence their operands.
void logicalOperatorsSequenceTest() {
  {
    A a;
    if (a.foo() > 0 && A(std::move(a)).foo() > 0) { // expected-note {{Assuming the condition is false}} expected-note {{Assuming the condition is false}} 
      // expected-note@-1 {{Left side of '&&' is false}} expected-note@-1 {{Left side of '&&' is false}}
			//expected-note@-2 {{Taking false branch}} expected-note@-2 {{Taking false branch}}
      A().bar();
    }
  }
  // A variation: Negate the result of the && (which pushes the && further down
  // into the AST).
  {
    A a;
    if (!(a.foo() > 0 && A(std::move(a)).foo() > 0)) { // expected-note {{Assuming the condition is false}} expected-note {{Assuming the condition is false}}
      // expected-note@-1 {{Left side of '&&' is false}} expected-note@-1 {{Left side of '&&' is false}}
      // expected-note@-2 {{Taking true branch}} expected-note@-2 {{Taking true branch}}
      A().bar();
    }
  }
  {
    A a;
    if (A(std::move(a)).foo() > 0 && a.foo() > 0) { // expected-warning {{Method called on moved-from object 'a'}} expected-note {{Method called on moved-from object 'a'}}
      // expected-note@-1 {{Object 'a' is moved}} expected-note@-1 {{Assuming the condition is true}} expected-note@-1 {{Assuming the condition is false}}
      // expected-note@-2 {{Left side of '&&' is false}} expected-note@-2 {{Left side of '&&' is true}}
      // expected-note@-3 {{Taking false branch}}
      A().bar();
    }
  }
  {
    A a;
    if (a.foo() > 0 || A(std::move(a)).foo() > 0) { // expected-note {{Assuming the condition is true}} 
			//expected-note@-1 {{Left side of '||' is true}}
			//expected-note@-2 {{Taking true branch}}
      A().bar();
    }
  }
  {
    A a;
    if (A(std::move(a)).foo() > 0 || a.foo() > 0) { // expected-warning {{Method called on moved-from object 'a'}} expected-note {{Method called on moved-from object 'a'}}
      // expected-note@-1 {{Object 'a' is moved}} expected-note@-1 {{Assuming the condition is false}} expected-note@-1 {{Left side of '||' is false}}
      A().bar();
    }
  }
}

// A range-based for sequences the loop variable declaration before the body.
void forRangeSequencesTest() {
  A v[2] = {A(), A()};
  for (A &a : v) {
    A b;
    b = std::move(a); // no-warning
  }
}

// If a variable is declared in an if statement, the declaration of the variable
// (which is treated like a reinitialization by the check) is sequenced before
// the evaluation of the condition (which constitutes a use).
void ifStmtSequencesDeclAndConditionTest() {
  for (int i = 0; i < 3; ++i) {
    if (A a = A()) {
      A b;
      b = std::move(a); // no-warning
    }
  }
}

struct C : public A {
  [[clang::reinitializes]] void reinit();
};

void subRegionMoveTest() {
  {
    A a;
    B b = std::move(a.b);
    a.b.foo();
#ifdef AGGRESSIVE
    // expected-note@-3{{Object 'b' is moved}}
    // expected-warning@-3{{Method called on moved-from object 'b'}}
    // expected-note@-4 {{Method called on moved-from object 'b'}}
#endif
  }
  {
    A a;
    A a1 = std::move(a);
    a.b.foo();
#ifdef AGGRESSIVE
    // expected-note@-3{{Calling move constructor for 'A'}}
    // expected-note@-4{{Returning from move constructor for 'A'}}
    // expected-warning@-4{{Method called on moved-from object 'b'}}
    // expected-note@-5{{Method called on moved-from object 'b'}}
#endif
  }
  // Don't report a misuse if any SuperRegion is already reported.
  {
    A a;
    A a1 = std::move(a); // expected-note {{Object 'a' is moved}}
    a.foo();             // expected-warning {{Method called on moved-from object 'a'}} expected-note {{Method called on moved-from object 'a'}}
    a.b.foo();           // no-warning
  }
  {
    C c;
    C c1 = std::move(c); // expected-note {{Object 'c' is moved}}
    c.foo();             // expected-warning {{Method called on moved-from object 'c'}} expected-note {{Method called on moved-from object 'c'}}
    c.b.foo();           // no-warning
  }
}

void resetSuperClass() {
  C c;
  C c1 = std::move(c);
  c.clear();
  C c2 = c; // no-warning
}

void resetSuperClass2() {
  C c;
  C c1 = std::move(c);
  c.reinit();
  C c2 = c; // no-warning
}

void reportSuperClass() {
  C c;
  C c1 = std::move(c); // expected-note {{Object 'c' is moved}}
  c.foo();             // expected-warning {{Method called on moved-from object 'c'}} expected-note {{Method called on moved-from object 'c'}}
  C c2 = c;            // no-warning
}

struct Empty {};

Empty inlinedCall() {
  // Used to warn because region 'e' failed to be cleaned up because no symbols
  // have ever died during the analysis and the checkDeadSymbols callback
  // was skipped entirely.
  Empty e{};
  return e; // no-warning
}

void checkInlinedCallZombies() {
  while (true)
    inlinedCall();
}

void checkLoopZombies() {
  while (true) {
    Empty e{};
    Empty f = std::move(e); // no-warning
  }
}

struct MoveOnlyWithDestructor {
  MoveOnlyWithDestructor();
  ~MoveOnlyWithDestructor();
  MoveOnlyWithDestructor(const MoveOnlyWithDestructor &m) = delete;
  MoveOnlyWithDestructor(MoveOnlyWithDestructor &&m);
};

MoveOnlyWithDestructor foo() {
  MoveOnlyWithDestructor m;
  return m;
}

class HasSTLField {
  std::vector<int> V;
  void foo() {
    // Warn even in non-aggressive mode when it comes to STL, because
    // in STL the object is left in "valid but unspecified state" after move.
    std::vector<int> W = std::move(V); // expected-note{{Object 'V' of type 'std::vector' is left in a valid but unspecified state after move}}
    V.push_back(123); // expected-warning{{Method called on moved-from object 'V'}}
                      // expected-note@-1{{Method called on moved-from object 'V'}}
  }
};

void localRValueMove(A &&a) {
  A b = std::move(a); // expected-note{{Object 'a' is moved}}
  a.foo(); // expected-warning{{Method called on moved-from object 'a'}}
           // expected-note@-1{{Method called on moved-from object 'a'}}
}
