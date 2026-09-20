#!/usr/bin/env python3
"""Deterministic stand-in for the TypeSafe Jev API, used by the regression tests.

The rules are fixed so the expected results in test/sql/*.test never move:

  noul   -> 0.9 when the LAST word of `state.condition` appears (case-insensitively)
            in the row JSON, else 0.1
  score  -> level index  = length of the row JSON modulo the number of levels
  choice -> option index = length of the row JSON modulo the number of options

  A condition containing "trigger422" answers 422 (a non-retryable error).
  A condition containing "trigger503" answers 503 (retryable, so it exhausts the retries).
  usage.input_tokens = len(request body) // 4

Run: python3 test/mock_api.py [port]        (default 18765)
"""
import json
import sys
from http.server import BaseHTTPRequestHandler, ThreadingHTTPServer

API_KEY = "jev-test-key"


class Handler(BaseHTTPRequestHandler):
    protocol_version = "HTTP/1.1"  # keep-alive, so connection reuse is exercised

    def log_message(self, *args):  # quiet
        pass

    def do_POST(self):
        body = self.rfile.read(int(self.headers.get("Content-Length", 0)))
        if self.headers.get("Authorization", "") != "Bearer " + API_KEY:
            return self._send(401, {"error": "invalid api key"})
        request = json.loads(body)
        state, questions = request["state"], request["questions"]
        rows = state.get("rows", [])
        condition = state.get("condition", "")
        if "trigger422" in condition:
            return self._send(422, {"error": "mock validation failure"})
        if "trigger503" in condition:
            return self._send(503, {"error": "mock overloaded"})
        needle = condition.split()[-1].lower() if condition.split() else ""

        answers = {}
        for question_id, question in questions.items():
            index = int(question_id[1:])
            row_json = json.dumps(rows[index], sort_keys=True)
            if question["type"] == "noul":
                hit = bool(needle) and needle in row_json.lower()
                answers[question_id] = {"type": "noul", "noul": 0.9 if hit else 0.1}
            elif question["type"] == "score":
                levels = question["criteria"]
                chosen = len(row_json) % len(levels)
                answers[question_id] = {
                    "type": "score",
                    "score": float(chosen),
                    "legend": {str(i): level for i, level in enumerate(levels)},
                    "probabilities": {str(i): (1.0 if i == chosen else 0.0) for i in range(len(levels))},
                    "confidence": 1.0,
                }
            elif question["type"] == "choice":
                options = list(question["criteria"].keys())
                chosen = len(row_json) % len(options)
                answers[question_id] = {
                    "type": "choice",
                    "choice": options[chosen],
                    "probabilities": {o: (1.0 if i == chosen else 0.0) for i, o in enumerate(options)},
                    "confidence": 1.0,
                }
            else:
                return self._send(400, {"error": "unknown question type"})

        self._send(200, {
            "model": request.get("model", "jev-mock"),
            "answers": answers,
            "usage": {"input_tokens": len(body) // 4, "output_tokens": len(answers)},
        })

    def _send(self, code, payload):
        data = json.dumps(payload).encode()
        self.send_response(code)
        self.send_header("Content-Type", "application/json")
        self.send_header("Content-Length", str(len(data)))
        self.end_headers()
        self.wfile.write(data)


if __name__ == "__main__":
    port = int(sys.argv[1]) if len(sys.argv) > 1 else 18765
    ThreadingHTTPServer(("127.0.0.1", port), Handler).serve_forever()
