package api

type Router struct {
    Prefix string
}

func NewRouter() Router {
    return Router{Prefix: "/api"}
}
