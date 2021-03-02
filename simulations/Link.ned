package dsbase.simulations;

channel Link extends ned.DelayChannel {
  parameters:
      delay = (1s);
      @link;
      int weight = default(1);
}