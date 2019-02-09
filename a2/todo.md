# To-Do
## Errors
- neg. cars in intersection
  - exiting not decreasing carsLeft properly
- entering from wrong dir. car not in
- hanging
  - at red since wrong # cars in intersection?

## Minor
- test simul. cars going through intersection
- ask about helgrind msgs
- add fxn def's in stop

# Stop
1. enter lane
  - mutex for lane data
  - no other car in lane
2. enter intersection
  - mutex for reading/writing intersection data
  - straight
    - 2 quads in front
  - left
    - 3 quads
  - right
    - 1 quad in front
3. exit intersection
  - keep queue of cars entering same

# Light
1. enter lane
2. enter intersection
3. exit intersection
