package testdata;

public class Add {
    Num x;
    Num y;

    Add(int xv, int yv)
    {
        x = new Num(xv);
        y = new Num(yv);
    }

    public int call()
    {
        return x.val + y.val;
    }
}
