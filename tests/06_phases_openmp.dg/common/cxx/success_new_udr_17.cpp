/*
<testinfo>
test_generator=config/mercurium-omp

test_compile_fail_nanox_plain=yes
test_compile_faulty_nanox_plain=yes

test_compile_fail_nanox_instrument=yes
test_compile_faulty_nanox_instrument=yes
</testinfo>
*/

enum A
{
  L=1,
  D=2
};

A& operator+=(const A& _in, A& _out)
{
  return  _out = A(_out + _in);
}

int main (int argc, char* argv[])
{
	enum A a;
	#pragma omp parallel reduction(+: a)
    a;

    return 0;
}

