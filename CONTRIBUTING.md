# Contributing

## Prefer Simplicity

Choose the simplest design that correctly solves the current problem.

Do not introduce abstractions, patterns, layers, or frameworks without a concrete need. Complexity should solve an actual problem rather than anticipate one that may never occur.

Follow KISS and YAGNI.

## Keep Responsibilities Focused

Classes and methods should have one clear responsibility and operate at a cohesive level of abstraction.

Do not use arbitrary line-count limits. Refactor when a class or method begins accumulating conceptually distinct responsibilities.

## Write Self-Explanatory Code

Prefer clear names and structure over explanatory comments.

Comments should primarily explain **why** something is done, especially when the reason is not obvious from the code.

Avoid comments that merely restate what the code already says.

If code requires a comment to explain what it does, first consider whether the code can be made clearer.

## Prefer Descriptive Names

Prefer full, descriptive names over abbreviations.

Use abbreviations only when they are conventional and unambiguous within the domain, such as `ID`, `API`, or `FOUP`.

Names should make intent clear without requiring additional context.

## Keep Tests Focused

Each test should verify one clear behavior or contract.

Tests should be easy to understand independently and should avoid unnecessary setup or dependencies.

## Limit Assertions per Test

As a general guideline, prefer no more than three meaningful assertions in a test.

If a test requires many assertions, first consider whether:

- the test should be split into multiple behaviors,
- repeated cases should be parameterized, or
- the underlying implementation has too many responsibilities and should be refactored.

This is a heuristic rather than an absolute rule.

## Parameterize Repeated Behavior

Use parameterized tests when the same behavior needs to be verified across multiple inputs.

Do not introduce parameterization merely to reduce the number of lines in a test.

## Do Not Reimplement Production Logic in Tests

Tests should verify production behavior against **independently established expected outcomes**.

Prefer explicit reference values, known state transitions, golden or reference datasets, externally verified results, or independently derived expectations.

Avoid calculating the expected result by reproducing the same algorithm or business logic used by the production implementation. Duplicating production logic in a test can allow the same conceptual error to exist in both implementations while the test still passes.

Prefer:

```cpp
EXPECT_EQ(calculator.calculate(knownInput), 42);
```

over implementing another version of the calculation inside the test to determine the expected value.

Test doubles should follow the same principle. Mocks, fakes, and stubs should provide simple, controlled behavior required by the test rather than reimplementing the production component they replace.

For example, a mock process module may be configured to return success or failure. It should not reproduce the real process module's decision-making logic.

## Preserve Clear Ownership and State

Every important piece of mutable state should have clear ownership.

Avoid multiple components independently modifying the same state unless there is a deliberate synchronization mechanism and a clear reason for doing so.

Components that perform an operation should report its outcome. The component responsible for application state should decide how that outcome changes the software model.

## Prefer Interfaces at Component Boundaries

Use interfaces where they provide a meaningful boundary between components or implementations.

Interfaces are especially useful for hardware abstraction, simulation, testing, and isolating external or legacy systems.

Do not create interfaces for every class by default.

## Prefer Standard Resource Management

Prefer RAII and standard C++ resource-management facilities.

Use automatic storage and standard containers where possible. Use smart pointers when dynamic ownership is required, and make ownership semantics explicit.

Avoid manual resource management unless there is a concrete reason for it.

## Optimize for Maintainability

Prefer code that another engineer can understand, test, debug, and safely modify over code that is merely clever or compact.

Architectural sophistication is valuable only when it makes the system easier to evolve or solves a real engineering constraint.

## Guiding Principle

Keep the system simple, focused, explicit, readable, and testable.

Introduce complexity only when the problem demonstrates that it is necessary.