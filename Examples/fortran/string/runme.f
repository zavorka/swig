! ----------------------------------------------------------------------
!  EXAMPLE: Calling a C function from fortran using swig.
!
!  The string example shows how to call a c funtion from Fortran
!
! ======================================================================

      program runme
        IMPLICIT NONE

        character (LEN=7) :: name="derrick"
        character*20 ret

        call sayhi(name, ret)
        write(*,*) "The result of sayhi is: ", ret

!       This case does not work
!        call sayhi("derrick", ret)
!        write(*,*) "The result of sayhi is: ", ret


!       Swig_exit(0)
      end program runme
