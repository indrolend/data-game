# Executable checks and their limits

The experimental audit helper implements:

1. All selected core state floats remain finite.
2. Player battery remains within [-0.01, 100.01].
3. Stored souls remain within [0, PHONE_CAPACITY].
4. Deposited souls remain within [0, requiredSouls].
5. Grounded player height agrees with available collision support within 0.35 world units.

The state digest includes selected frame, room, player, camera, objective, and target fields. It is **not a full-memory or whole-program deterministic proof**.

Negative controls exercised in the sandbox: battery set to 150 should fail the battery invariant; altered movement input should produce a replay digest divergence.

The formal gameplay transition sequence to test next:

`enemy alive → defeated → soul exposed → attracted → stored → projectile fired → capture slot filled → room clear → room transition`

Required future properties:
- Every fired projectile has a terminal outcome or justified bounded lifetime.
- Deposits increase only on valid capture-slot hits.
- Room completion requires the specified number of filled slots.
- Forward room transition follows its required clearance and traversal conditions.
- Replayed identical inputs from identical initial conditions reproduce selected state and corresponding original renderer output.

These are **proposed contracts** until implemented and verified. Temporal bounds must be derived from actual game rules, not guessed.
