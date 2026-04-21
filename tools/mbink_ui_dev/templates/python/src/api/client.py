class ApiClient:
    def __init__(self, base_url: str = 'http://localhost:8000') -> None:
        self.base_url = base_url

    def describe(self) -> str:
        return f'python api client -> {self.base_url}'
