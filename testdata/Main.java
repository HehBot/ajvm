package testdata;
import java.lang.System;

public class Main {
    public static void main() {

        int res = Add.perform(2, 3);
        System.out.println(res);

        Add a = new Add(4, 5);
        System.out.println(a.call(7));

        ExtendedAdd b = new ExtendedAdd(9, 10, 11, 12);
        Add c = b;
        System.out.println(b.call(13));
        System.out.println(c.call(13));

        System.out.println(3.1415);
        if (b.call(13) == 56)
            System.out.println(9);
        else
            System.out.println(10);
    }
}
