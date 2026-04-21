def get_env(name: str, default: str = '') -> str:
    import os

    return os.environ.get(name, default)
