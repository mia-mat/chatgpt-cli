*I don't like leaving the comfort of my shell >w<*

**A simple CLI for interacting with OpenAI's ChatGPT models.**

## Configuration

Default settings may be read from a configuration file:

* **Linux/macOS:** `~/.config/chatgpt-cli.conf`
* **Windows:** `C:\Users\<username>\AppData\Local\chatgpt-cli\config`

Format the file with `KEY=VALUE` pairs (use `|` to escape newlines in values):

```
MODEL=gpt-5-mini
INSTRUCTIONS=You are a helpful assistant.|
Respond concisely.
```

## Environment Variables

* `CHATGPT_CLI_API_KEY` – Your OpenAI API key (used if not provided via `--key`).

## Usage

```bash
./chatgpt_cli [OPTIONS] PROMPT...
```

### Required Options

* `-k, --key API_KEY` – OpenAI API key (overrides environment variable)
* `-m, --model MODEL` – Model to use (overrides config)

### Optional Options

* `-i, --instructions TEXT` – System instructions for the model (overrides config)
* `-r, --raw` – Print raw JSON response instead of parsed text
* `-h, --help` – Show help message