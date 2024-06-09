package testdata;

public class Add {

    int x, y;
    Add(int xv, int yv)
    {
        x = xv;
        y = yv;
    }
    public int call(int z)
    {
        return x + y + z;
    }

    public static int perform(int xv, int yv)
    {
        return xv + yv;
    }
}
