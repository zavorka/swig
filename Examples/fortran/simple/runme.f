! ----------------------------------------------------------------------
!  EXAMPLE: Calling a C function from fortran using swig.
!
!  This simple example shows how to call a c funtion from Fortran
!
! ======================================================================
!  AUTHOR:  Derrick Kearney, Purdue University
!  Copyright (c) 2005-2010  Purdue Research Foundation
!
!  See the file "license.terms" for information on usage and
!  redistribution of this file, and for a DISCLAIMER OF ALL WARRANTIES.
! ======================================================================

      program runme
        IMPLICIT NONE

        integer gcd, a, b , g

        character*5 str
        character*20 ret

        a = 45
        b = 105
        g = 0

        g = gcd(a, b)
        write(*,*) "The gcd of ", a," and ", b, " is ", g

        call sayhi(str, a, ret)
        write(*,*) "The result of sayhi is ", ret


!       Swig_exit(0)
      end program runme
