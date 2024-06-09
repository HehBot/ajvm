package testdata;
import java.lang.System;

public class Main {
    public static void main() {

        int res = Add.perform(2, 3);
        System.out.println(res);

        Add a = new Add(4, 5);
        System.out.println(a.call(7));
    }
}
