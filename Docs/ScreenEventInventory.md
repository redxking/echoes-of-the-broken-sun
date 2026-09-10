# Screen-Wide Event and Feedback Inventory

**Author and owner:** Angelis Pseftis
**Maintained:** 2026-09-10
**Standing:** Required by `DeliveryPlan.md` Phase D1. Inventories every player-visible screen, widget, world-space marker, and effect.

Every visible control/indicator/effect has a coverage row; unbound or unexplained effects remain unfinished work.

| Event Family | Authoritative Event/State | Permitted Observer | World/UI Location | Priority | Stacking/Replacement Rule | Audio | Recovery Behavior |
|---|---|---|---|---|---|---|---|
| Selection/Order | `OnEntitySelected`, `OnOrderDispatched`, `OnOrderRejected` | Controlling Player | World (Selection Circle, Target Marker), HUD (Unit Info) | Medium | Replaces previous selection marker; failed action plays distinct rejection sound. | Selection acknowledgment; distinct denial cue. | Resets upon deselection or valid order. |
| Gather/Deposit | `OnResourceExtracted`, `OnResourceDeposited`, `OnDepositExhausted` | Controlling Player | World (Worker Model, Node), HUD (Resource Counter) | Low | Overlapping numbers coalesce; extraction beam is continuous while working. | Subtle extraction hum; credit chime on deposit. | Clears upon task completion/cancellation. |
| Idle/Stranded | `OnWorkerIdle`, `OnPathBlocked` | Controlling Player | HUD (Idle Worker Alert) | Medium | Alerts stack up to 3, older ones coalesce into "Multiple Idle". | Distinct idle notification beep. | Clears when worker is assigned a valid task. |
| Construction | `OnBuildStarted`, `OnBuildProgress`, `OnBuildCompleted` | All (if visible) | World (Holographic frame, progress bar), HUD (Queue) | Medium | Replaces blueprint with active frame. Only one completion notification per building. | Welding/assembly noise; completion fanfare. | Persists through save/load; resets on cancel. |
| Upgrade/Tech | `OnResearchStarted`, `OnResearchCompleted` | Controlling Player | World (Model Detail), HUD (Queue/Upgrades) | High | Persistent visual cue replaces base model detail; does not stack with same tier. | Researching hum; prominent completion chime. | Permanent modification; persists through save/load. |
| Fire/Hit/Damage | `OnWeaponFired`, `OnDamageReceived`, `OnShieldHit` | All (if visible) | World (Projectile, Hit Impact, Shield Flare) | High | Projectiles exist per shot; damage states trigger at <50% and <30% HP (does not stack, replaces). | Weapon specific report; impact crunch/flare. | Remains until repaired; destruction overrides. |
| Death/Destruction | `OnEntityDestroyed`, `OnStructureCollapsed` | All (if visible) | World (Debris, Explosion) | Critical | Overrides all other states and animations; debris persists briefly then fades. | Destruction explosion/collapse roar. | Replaces entity with dead state; releases cap. |
| Power/Logistics | `OnPowerLost`, `OnLogisticsCapped`, `OnInsolvency` | Controlling Player | World (Offline Visuals), HUD (Supply Warning) | Critical | Overrides operational state; Logistics warning pulses red if cap reached. | Power down whine; sharp capacity warning. | Recovers instantly when power/capacity restored. |
| Scouting/Attack | `OnHostileContact`, `OnUnderAttack` | Controlling Player | Minimap (Ping), HUD (Alert History) | Critical | Coalesces multiple attacks in same sector into one sector ping; max 1 alert per 5s. | "Under attack" voice line/klaxon. | Ping fades after 3s; history remains retrievable. |
| Future Well | `OnWellContested`, `OnWellCaptured`, `OnWellExpired` | All (if visible) | World (Well Aura, Timer), HUD (Objective) | High | Well aura replaces neutral state; capture progress bar. | Deep resonant hum during capture; transition boom. | State is permanent for Reshape/Preserve; expires for Harvest. |
| Save/Network | `OnSaveProgress`, `OnDisconnect`, `OnHostMigration` | Controlling Player | HUD (System Overlay) | Critical | Full screen overlay; disables inputs until resolved. | System beep. | Restores control upon success or graceful exit. |

