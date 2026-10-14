// SPDX-License-Identifier: GPL-3.0-only
// Project gestures use only the actual panel, including name and overwrite dialogs.
import { B, EN } from "./workflow_device.mjs";
export function projectPage(d, slot) {
  d.tap(B.HOME);
  d.tap(B.SAVE);
  d.tap(B.SAVE);
  d.turn(EN.K1, -3);
  d.turn(EN.K1, slot);
}
export function setName(d, name) {
  d.turn(EN.K1, 12);
  for (let i = 0; i < 12; i++) d.key(8); // C# DELETE
  for (const char of name) {
    d.turn(EN.K2, char.charCodeAt(0) - 64);
    d.turn(EN.K1, 1);
  }
}
export function proposeSave(d, slot, name) {
  projectPage(d, slot);
  d.turn(EN.K4, 1);
  d.tap(B.OCTUP);
  if (d.ex.web_qa_confirmation()) d.tap(B.OCTUP);
  if (!d.ex.web_qa_name_open()) throw new Error("Project SAVE did not open NAME");
  setName(d, name);
  d.tap(B.OCTUP);
}
export function saveProject(d, slot, name) {
  proposeSave(d, slot, name);
  if (d.ex.web_qa_consent()) d.tap(B.SAVE);
  d.render(300);
}
export function loadProject(d, slot) {
  projectPage(d, slot);
  d.turn(EN.K3, 1);
  d.tap(B.OCTUP);
  d.render(300);
}
