! <testinfo>
! test_ignore=yes
! </testinfo>
PROGRAM P
    IMPLICIT NONE
    INTEGER :: X, Y

    X = 1
    Y = 2

    !$OMP TASK SHARED(X)
      X = X + 3
      Y = Y + 9
    !$OMP END TASK
    !$OMP TASKWAIT

    IF (X /= 4) STOP 1
    IF (Y /= 2) STOP 2
END PROGRAM P