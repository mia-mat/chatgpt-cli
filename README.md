*I don't like leaving the comfort of my shell >w<*

**A simple CLI for interacting with OpenAI's ChatGPT models.**

## Configuration

Default settings may be read from a configuration file:

* **Linux:** `~/.chatgpt-cli/.config`
* **Windows:** `C:\Users\<username>\AppData\Local\chatgpt-cli\.config`

Format the file with `KEY=VALUE` pairs.  
You may use `\` to escape newlines in values,  
and `#` at the start of a line to denote a comment.  
Empty lines are ignored.

```
model=gpt-5-mini

# Here is a comment!
instructions=You are a helpful assistant. \ 
Respond concisely.
```

## Environment Variables

* `CHATGPT_CLI_API_KEY` – Your OpenAI API key (used if not provided via `--key`).

## Usage

```bash
./chatgpt_cli [OPTIONS] PROMPT...
```

### Required

* `-k, --key API_KEY` – OpenAI API key (overrides `OPENAI_API_KEY` environment variable)
* `-m, --model MODEL` – OpenAI model to use (overrides `model` config option)

### Optional

* `-h, --help` – Show help message
* `-v, --version` – Show program version<br><br>

* `-r, --raw` – Print raw JSON response instead of parsed text (does not support streaming)
* `-R, --response-id` – Print the response id after completion (use with -H later)<br><br>

* `-H, --history [ID]` –Specify an OpenAI previous_response_id (defaults to last response's id)

* `-i, --instructions TEXT` – System instructions for the model (overrides `instructions` config option)
* `-t, --temperature DOUBLE` – Sampling temperature for the model, must be in [0,2] (overrides `temperature` config option)
* `-T, --max-tokens UINT64` – Upper bound for output tokens in the response (overrides `max-tokens` config option)




