/*
 * Nedflix iOS - Settings View Controller
 */

import UIKit

class SettingsViewController: UITableViewController {

    // MARK: - Properties

    private var settings: AppSettings {
        get { SettingsManager.shared.settings }
        set { SettingsManager.shared.settings = newValue }
    }

    private enum Section: Int, CaseIterable {
        case server
        case playback
        case about

        var title: String {
            switch self {
            case .server: return "Server"
            case .playback: return "Playback"
            case .about: return "About"
            }
        }
    }

    // MARK: - Lifecycle

    override func viewDidLoad() {
        super.viewDidLoad()
        title = "Settings"
        tableView.backgroundColor = UIColor(red: 0.04, green: 0.04, blue: 0.04, alpha: 1.0)
        tableView.register(UITableViewCell.self, forCellReuseIdentifier: "Cell")
        tableView.register(TextFieldCell.self, forCellReuseIdentifier: TextFieldCell.reuseId)
    }

    // MARK: - Table View Data Source

    override func numberOfSections(in tableView: UITableView) -> Int {
        return Section.allCases.count
    }

    override func tableView(_ tableView: UITableView, numberOfRowsInSection section: Int) -> Int {
        switch Section(rawValue: section)! {
        case .server: return 2
        case .playback: return 3
        case .about: return 2
        }
    }

    override func tableView(_ tableView: UITableView, titleForHeaderInSection section: Int) -> String? {
        return Section(rawValue: section)?.title
    }

    override func tableView(_ tableView: UITableView, cellForRowAt indexPath: IndexPath) -> UITableViewCell {
        switch Section(rawValue: indexPath.section)! {
        case .server:
            return serverCell(for: indexPath)
        case .playback:
            return playbackCell(for: indexPath)
        case .about:
            return aboutCell(for: indexPath)
        }
    }

    // MARK: - Cell Builders

    private func serverCell(for indexPath: IndexPath) -> UITableViewCell {
        switch indexPath.row {
        case 0:
            let cell = tableView.dequeueReusableCell(withIdentifier: TextFieldCell.reuseId, for: indexPath) as! TextFieldCell
            cell.configure(title: "Server URL", value: settings.serverURL, placeholder: "http://192.168.1.100:8080") { [weak self] value in
                self?.settings.serverURL = value
                APIClient.shared.configure(serverURL: value)
            }
            return cell
        case 1:
            let cell = tableView.dequeueReusableCell(withIdentifier: "Cell", for: indexPath)
            cell.textLabel?.text = "Test Connection"
            cell.textLabel?.textColor = UIColor(red: 0.9, green: 0.04, blue: 0.08, alpha: 1.0)
            cell.backgroundColor = .clear
            return cell
        default:
            return UITableViewCell()
        }
    }

    private func playbackCell(for indexPath: IndexPath) -> UITableViewCell {
        let cell = tableView.dequeueReusableCell(withIdentifier: "Cell", for: indexPath)
        cell.backgroundColor = .clear
        cell.textLabel?.textColor = .white
        cell.detailTextLabel?.textColor = .gray

        switch indexPath.row {
        case 0:
            cell.textLabel?.text = "Video Quality"
            cell.detailTextLabel?.text = settings.videoQuality.displayName
            cell.accessoryType = .disclosureIndicator
        case 1:
            cell.textLabel?.text = "Auto-Play"
            let toggle = UISwitch()
            toggle.isOn = settings.autoPlay
            toggle.onTintColor = UIColor(red: 0.9, green: 0.04, blue: 0.08, alpha: 1.0)
            toggle.addTarget(self, action: #selector(autoPlayChanged(_:)), for: .valueChanged)
            cell.accessoryView = toggle
        case 2:
            cell.textLabel?.text = "Show Subtitles"
            let toggle = UISwitch()
            toggle.isOn = settings.showSubtitles
            toggle.onTintColor = UIColor(red: 0.9, green: 0.04, blue: 0.08, alpha: 1.0)
            toggle.addTarget(self, action: #selector(subtitlesChanged(_:)), for: .valueChanged)
            cell.accessoryView = toggle
        default:
            break
        }

        return cell
    }

    private func aboutCell(for indexPath: IndexPath) -> UITableViewCell {
        let cell = tableView.dequeueReusableCell(withIdentifier: "Cell", for: indexPath)
        cell.backgroundColor = .clear
        cell.textLabel?.textColor = .white
        cell.detailTextLabel?.textColor = .gray
        cell.selectionStyle = .none

        switch indexPath.row {
        case 0:
            cell.textLabel?.text = "Version"
            cell.detailTextLabel?.text = "1.0.0"
        case 1:
            cell.textLabel?.text = "Platform"
            cell.detailTextLabel?.text = "iOS"
        default:
            break
        }

        return cell
    }

    // MARK: - Table View Delegate

    override func tableView(_ tableView: UITableView, didSelectRowAt indexPath: IndexPath) {
        tableView.deselectRow(at: indexPath, animated: true)

        switch Section(rawValue: indexPath.section)! {
        case .server:
            if indexPath.row == 1 {
                testConnection()
            }
        case .playback:
            if indexPath.row == 0 {
                showQualityPicker()
            }
        case .about:
            break
        }
    }

    // MARK: - Actions

    @objc private func autoPlayChanged(_ sender: UISwitch) {
        settings.autoPlay = sender.isOn
    }

    @objc private func subtitlesChanged(_ sender: UISwitch) {
        settings.showSubtitles = sender.isOn
    }

    private func testConnection() {
        let alert = UIAlertController(title: "Testing...", message: nil, preferredStyle: .alert)
        present(alert, animated: true)

        // Simulated test - would actually ping server
        DispatchQueue.main.asyncAfter(deadline: .now() + 1) { [weak self] in
            alert.dismiss(animated: true) {
                let result = UIAlertController(
                    title: self?.settings.serverURL.isEmpty == false ? "Connection OK" : "Not Configured",
                    message: nil,
                    preferredStyle: .alert
                )
                result.addAction(UIAlertAction(title: "OK", style: .default))
                self?.present(result, animated: true)
            }
        }
    }

    private func showQualityPicker() {
        let alert = UIAlertController(title: "Video Quality", message: nil, preferredStyle: .actionSheet)

        for quality in VideoQuality.allCases {
            let action = UIAlertAction(title: quality.displayName, style: .default) { [weak self] _ in
                self?.settings.videoQuality = quality
                self?.tableView.reloadData()
            }
            if quality == settings.videoQuality {
                action.setValue(true, forKey: "checked")
            }
            alert.addAction(action)
        }

        alert.addAction(UIAlertAction(title: "Cancel", style: .cancel))
        present(alert, animated: true)
    }
}

// MARK: - TextFieldCell

class TextFieldCell: UITableViewCell {
    static let reuseId = "TextFieldCell"

    private let titleLabel: UILabel = {
        let label = UILabel()
        label.textColor = .white
        label.font = .systemFont(ofSize: 16)
        return label
    }()

    private let textField: UITextField = {
        let tf = UITextField()
        tf.textColor = .white
        tf.textAlignment = .right
        tf.autocapitalizationType = .none
        tf.autocorrectionType = .no
        tf.keyboardType = .URL
        tf.returnKeyType = .done
        return tf
    }()

    private var onChange: ((String) -> Void)?

    override init(style: UITableViewCell.CellStyle, reuseIdentifier: String?) {
        super.init(style: style, reuseIdentifier: reuseIdentifier)
        setupUI()
    }

    required init?(coder: NSCoder) {
        fatalError("init(coder:) has not been implemented")
    }

    private func setupUI() {
        backgroundColor = .clear
        selectionStyle = .none

        contentView.addSubview(titleLabel)
        contentView.addSubview(textField)

        titleLabel.translatesAutoresizingMaskIntoConstraints = false
        textField.translatesAutoresizingMaskIntoConstraints = false

        NSLayoutConstraint.activate([
            titleLabel.leadingAnchor.constraint(equalTo: contentView.leadingAnchor, constant: 16),
            titleLabel.centerYAnchor.constraint(equalTo: contentView.centerYAnchor),

            textField.leadingAnchor.constraint(equalTo: titleLabel.trailingAnchor, constant: 16),
            textField.trailingAnchor.constraint(equalTo: contentView.trailingAnchor, constant: -16),
            textField.centerYAnchor.constraint(equalTo: contentView.centerYAnchor),
            textField.widthAnchor.constraint(greaterThanOrEqualToConstant: 150)
        ])

        textField.addTarget(self, action: #selector(textChanged), for: .editingChanged)
    }

    func configure(title: String, value: String, placeholder: String, onChange: @escaping (String) -> Void) {
        titleLabel.text = title
        textField.text = value
        textField.placeholder = placeholder
        self.onChange = onChange
    }

    @objc private func textChanged() {
        onChange?(textField.text ?? "")
    }
}
