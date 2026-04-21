package service

type Counter struct {
    value int
}

func (c *Counter) Increment() int {
    c.value++
    return c.value
}
